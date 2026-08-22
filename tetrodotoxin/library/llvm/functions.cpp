// Perimortem Engine
// Copyright © Matt Kaes

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/Function.h")
#include "llvm/IR/Function.h"
#else
#error LLVM Function is required by the Library native compiler
#endif

#include "llvm-c/Core.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "tetrodotoxin/library/llvm/carriers.hpp"
#include "tetrodotoxin/library/llvm/export.hpp"
#include "tetrodotoxin/library/llvm/functions.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/library/llvm/symbol.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto select_program(Ttx::Concept::Abstract& program)
    -> Core::Option<Tetrodotoxin::Library::Llvm::Program&> {
  return program.select<Tetrodotoxin::Library::Llvm::Program>();
}

static auto is_local_definition(
    const Tetrodotoxin::Library::Llvm::Unit& unit,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> current(
      definition.get_host());
  while (true) {
    auto source = current.get().select<Language::Types::Source>();
    if (source) {
      return unit.owns(source->get_host());
    }
    auto composite = current.get().select<Language::Types::Composite>();
    BAIL_IF(!composite);
    current = composite->get_definition().get_host();
  }
}

static auto get_program(Ttx::Concept::Abstract& body)
    -> Ttx::Concept::Abstract& {
  auto selected = body.select<Tetrodotoxin::Library::Llvm::Body>();
  return selected ? selected->get_program() : body;
}

static auto select_carriers(Ttx::Concept::Abstract& program)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  auto native = program.select<Tetrodotoxin::Library::Llvm::Program>();
  return native ? Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&>(
                      native->get_carriers())
                : Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&>();
}

static auto get_context(Tetrodotoxin::Library::Llvm::Program& program)
    -> llvm::LLVMContext& {
  return *llvm::unwrap(&program.get_context());
}

static auto get_module(Tetrodotoxin::Library::Llvm::Program& program)
    -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto get_builder(Tetrodotoxin::Library::Llvm::Body& body)
    -> llvm::IRBuilder<>& {
  return *llvm::unwrap(body.get_builder());
}

static auto fail_backend(
    Ttx::Concept::Abstract& program,
    Core::View::Bytes message) -> Bool {
  auto target = select_program(program);
  return target ? target->fail_backend(message) : False;
}

static auto fail_callable(
    Ttx::Concept::Abstract& program,
    Core::Option<const Tetrodotoxin::Language::Definition&> definition,
    Core::View::Bytes message,
    Core::View::Bytes hint = {}) -> Bool {
  auto target = select_program(program);
  auto anchor = definition.visit(
      []() { return Core::Option<Ttx::Lexical::Anchor>(); },
      [](const Tetrodotoxin::Language::Definition& selected) {
        return Core::Option<Ttx::Lexical::Anchor>(selected.get_anchor());
      });
  if (target && anchor) {
    return target->fail_source(*anchor, message, hint);
  }

  return fail_backend(program, message);
}

static auto get_attribute_text(
    const Tetrodotoxin::Language::Attribute& attribute)
    -> Core::Option<const Core::View::Bytes&> {
  const Core::View::Bytes* selected =
      attribute.get_value().find<Core::View::Bytes>();
  return selected ? Core::Option<const Core::View::Bytes&>(*selected)
                  : Core::Option<const Core::View::Bytes&>();
}

static auto select_function_attributes(
    Ttx::Concept::Abstract& program,
    const Tetrodotoxin::Language::Definition& definition,
    Core::Option<Core::View::Bytes>& abi,
    Core::Option<Core::View::Bytes>& symbol) -> Bool {
  for (const Tetrodotoxin::Language::Attribute& attribute :
       definition.get_attributes()) {
    Core::View::Bytes key = attribute.get_key();
    if (key != "abi"_view && key != "symbol"_view) {
      continue;
    }

    auto text = get_attribute_text(attribute);
    if (!text) {
      return fail_callable(
          program, definition,
          "LLVM ABI Attributes require one string value."_view);
    }

    if (key == "abi"_view) {
      if (abi) {
        return fail_callable(
            program, definition,
            "LLVM accepts at most one ABI Attribute per Callable."_view);
      }

      abi = *text;
    } else if (symbol) {
      return fail_callable(
          program, definition,
          "LLVM accepts at most one symbol Attribute per Callable."_view);
    } else {
      symbol = *text;
    }
  }

  if (!abi && !symbol) {
    return True;
  }

  if (!abi || *abi != "C"_view) {
    return fail_callable(
        program, definition,
        symbol
            ? "A native symbol override requires `@abi(\"C\")`."_view
            : "LLVM supports only the C ABI requested by this Callable."_view);
  }

  if (symbol && !Llvm::Symbol::validate(*symbol)) {
    return fail_callable(
        program, definition,
        "The native symbol is not a valid C identifier."_view,
        "Use a nonempty identifier containing letters, digits, or `_`."_view);
  }

  return True;
}

static auto declares_self(const Ttx::Model::Callable& callable) -> Bool {
  auto first = callable.get_parameters().get_abstract(0);
  auto parameter = first ? first->select<Ttx::Model::Addressable>()
                         : Core::Option<const Ttx::Model::Addressable&>();
  return parameter && parameter->get_name() == "self"_view;
}

static auto uses_memory_abi(
    Tetrodotoxin::Library::Llvm::Program& program,
    llvm::Type& type) -> Bool {
  llvm::Module& module = get_module(program);
  return Bool(
      type.isAggregateType() &&
      module.getDataLayout().getTypeAllocSize(&type).getFixedValue() > 16);
}

static auto get_extension(
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Type& type) -> Core::Option<llvm::Attribute::AttrKind> {
  auto native = carriers.get_type(type);
  if (!native) {
    return {};
  }

  auto integer = llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(*native));
  if (!integer || integer->getBitWidth() >= 32 || carriers.is_real(type)) {
    return {};
  }

  return carriers.is_signed(type) ? llvm::Attribute::SExt
                                  : llvm::Attribute::ZExt;
}

static auto select_parameter_types(
    Ttx::Concept::Abstract& program,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Callable& callable,
    Memory::Dynamic::Vector<llvm::Type*>& native,
    Memory::Dynamic::Vector<const Ttx::Model::Type*>& semantic) -> Bool {
  const Ttx::Concept::Layout& layout = callable.get_parameters();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    auto parameter = entry ? entry->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
    if (!parameter) {
      return fail_backend(
          program,
          "LLVM received a Callable parameter without an Addressable."_view);
    }

    auto type = carriers.get_type(parameter->get_type());
    if (!type) {
      return fail_backend(
          program,
          "LLVM cannot find the completed carrier for a Callable parameter."_view);
    }

    native.insert(llvm::unwrap(*type));
    semantic.insert(&parameter->get_type());
  }

  return True;
}

static auto select_result_types(
    Ttx::Concept::Abstract& program,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Callable& callable,
    Memory::Dynamic::Vector<llvm::Type*>& native,
    Memory::Dynamic::Vector<const Ttx::Model::Type*>& semantic) -> Bool {
  const Ttx::Concept::Layout& layout = callable.get_results();
  auto library_callable = callable.select<Language::Model::Callable>();
  auto self_result = library_callable
                         ? library_callable->get_self_result()
                         : Core::Option<const Ttx::Model::Addressable&>();
  auto target = select_program(program);
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    auto addressable = entry ? entry->select<Ttx::Model::Addressable>()
                             : Core::Option<const Ttx::Model::Addressable&>();
    auto type =
        addressable
            ? Core::Option<const Ttx::Model::Type&>(addressable->get_type())
        : entry ? entry->resolve().select<Ttx::Model::Type>()
                : Core::Option<const Ttx::Model::Type&>();
    if (!type || !target) {
      return fail_backend(
          program,
          "LLVM received a Callable result without an exact Type."_view);
    }

    llvm::Type* carrier =
        self_result && addressable && &*self_result == &*addressable
            ? llvm::PointerType::getUnqual(get_context(*target))
            : carriers.get_type(*type).visit(
                  []() -> llvm::Type* { return nullptr; },
                  [](LLVMTypeRef selected) -> llvm::Type* {
                    return llvm::unwrap(selected);
                  });
    if (!carrier) {
      return fail_backend(
          program,
          "LLVM cannot find the completed carrier for a Callable result."_view);
    }

    native.insert(carrier);
    semantic.insert(&*type);
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Functions::reserve(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable,
    Record record) const -> Core::Option<Bool> {
  auto found = records.find(&callable);
  if (found) {
    if (found->value.kind != record.kind) {
      fail_backend(
          program,
          "LLVM received different owners for one Callable identity."_view);
      return {};
    }

    return False;
  }

  records.insert(&callable, record);
  return True;
}

auto Tetrodotoxin::Library::Llvm::Functions::reserve_function(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable,
    const Tetrodotoxin::Language::Definition& definition) const
    -> Core::Option<Bool> {
  auto target = select_program(program);
  if (!target || !target->get_unit().is_package_member() ||
      is_local_definition(target->get_unit(), definition)) {
    return reserve(
        program, callable, Record(Kind::Function, {}, {}, definition));
  }

  auto symbol = target->get_unit().find(callable);
  if (!symbol) {
    fail_callable(
        program, definition,
        "LLVM Package member is missing one external Callable binding."_view);
    return {};
  }
  return reserve(
      program, callable, Record(Kind::External, {}, *symbol, definition));
}

auto Tetrodotoxin::Library::Llvm::Functions::reserve_foreign(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable,
    Core::View::Bytes abi,
    Core::View::Bytes symbol) const -> Core::Option<Bool> {
  if (abi != "C"_view) {
    fail_callable(
        program, {}, "LLVM supports only the authored Foreign C ABI."_view);
    return {};
  }

  if (!Llvm::Symbol::validate(symbol)) {
    fail_callable(
        program, {},
        "The Foreign Callable symbol is not a valid C identifier."_view);
    return {};
  }

  auto target = select_program(program);
  BAIL_IF(!target);

  auto reserved =
      reserve(program, callable, Record(Kind::Foreign, abi, symbol));
  if (reserved && *reserved) {
    foreign_callables.insert(callable);
    if (!target->add_import(
            Tetrodotoxin::Linker::Import(
                Tetrodotoxin::Linker::Import::Kind::Function, abi, symbol))) {
      return {};
    }
  }

  return reserved;
}

auto Tetrodotoxin::Library::Llvm::Functions::reserve_construction(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& owner,
    Bool provider,
    Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Addressable>>
        parameters) const -> Bool {
  auto target = select_program(program);
  BAIL_IF(!target);

  auto found = constructions.find(&owner);
  if (found) {
    if (found->value.provider != provider ||
        found->value.parameters.get_size() != parameters.get_size()) {
      return fail_backend(
          program,
          "LLVM Type construction changed its reserved target facts."_view);
    }
    for (Count index = 0; index < parameters.get_size(); index++) {
      if (&found->value.parameters[index].get() !=
          &parameters.get_data()[index].get()) {
        return fail_backend(
            program,
            "LLVM Type construction changed its reserved target facts."_view);
      }
    }
    return True;
  }

  auto external = target->get_unit().find(owner);
  if (!provider && !external) {
    return fail_backend(
        program,
        "LLVM Package member is missing one external Type construction "
        "binding."_view);
  }

  Symbol generated(
      target->get_arena(), owner, Symbol::Kind::Construction,
      target->get_unit());
  Core::View::Bytes symbol = external ? *external : generated.get_view();
  ConstructionRecord record(provider, symbol);
  for (const Ttx::Concept::Reference<const Ttx::Model::Addressable>& parameter :
       parameters) {
    record.parameters.insert(parameter);
  }
  constructions.insert(&owner, static_cast<ConstructionRecord&&>(record));
  return True;
}

auto Tetrodotoxin::Library::Llvm::Functions::complete_construction(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& owner) const -> Bool {
  auto target = select_program(program);
  auto carriers = select_carriers(program);
  auto found = constructions.find(&owner);
  if (!target || !carriers || !found) {
    return fail_backend(
        program,
        "LLVM cannot complete Type construction before reservation."_view);
  }

  ConstructionRecord& record = found->value;
  if (record.completed) {
    return True;
  }

  auto result = carriers->get_type(owner);
  if (!result) {
    return fail_backend(
        program,
        "LLVM cannot complete Type construction without its result carrier."_view);
  }

  llvm::LLVMContext& context = get_context(*target);
  llvm::Module& module = get_module(*target);
  Bool sret = uses_memory_abi(*target, *llvm::unwrap(*result));
  Memory::Dynamic::Vector<llvm::Type*> native_parameters;
  if (sret) {
    native_parameters.insert(llvm::PointerType::getUnqual(context));
    record.sret_type = *result;
  }

  record.indirect_parameters.clear();
  for (const Ttx::Concept::Reference<const Ttx::Model::Addressable>& retained :
       record.parameters.get_view()) {
    const Ttx::Model::Type& type = retained.get().get_type();
    auto native = carriers->get_type(type);
    if (!native) {
      return fail_backend(
          program,
          "LLVM cannot complete Type construction without every Field "
          "carrier."_view);
    }
    Bool indirect = uses_memory_abi(*target, *llvm::unwrap(*native));
    record.indirect_parameters.insert(indirect);
    native_parameters.insert(
        indirect ? llvm::PointerType::getUnqual(context)
                 : llvm::unwrap(*native));
    native_parameters.insert(llvm::Type::getInt1Ty(context));
  }

  llvm::FunctionType* signature = llvm::FunctionType::get(
      sret ? llvm::Type::getVoidTy(context) : llvm::unwrap(*result),
      llvm::ArrayRef<llvm::Type*>(
          native_parameters.get_data(), native_parameters.get_size()),
      false);
  llvm::GlobalValue::LinkageTypes linkage =
      target->get_unit().is_package_member()
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage;
  if (module.getNamedValue(llvm_text(record.symbol))) {
    return fail_backend(
        program,
        "LLVM Type construction symbol collides with another declaration."_view);
  }
  llvm::Function& function = *llvm::Function::Create(
      signature, linkage, llvm_text(record.symbol), module);
  if (target->get_unit().is_package_member()) {
    function.setVisibility(llvm::GlobalValue::HiddenVisibility);
  }

  Count offset = sret ? 1 : 0;
  if (sret) {
    function.addParamAttr(
        0,
        llvm::Attribute::getWithStructRetType(context, llvm::unwrap(*result)));
    function.addParamAttr(0, llvm::Attribute::NoAlias);
    function.addParamAttr(
        0, llvm::Attribute::getWithAlignment(
               context,
               module.getDataLayout().getABITypeAlign(llvm::unwrap(*result))));
  }
  for (Count index = 0; index < record.parameters.get_size(); index++) {
    const Ttx::Model::Type& type = record.parameters[index].get().get_type();
    auto native = carriers->get_type(type);
    BAIL_IF(!native);
    Count parameter = offset + index * 2;
    if (record.indirect_parameters[index]) {
      function.addParamAttr(
          Unsigned_32(parameter),
          llvm::Attribute::getWithByValType(context, llvm::unwrap(*native)));
      function.addParamAttr(
          Unsigned_32(parameter),
          llvm::Attribute::getWithAlignment(
              context,
              module.getDataLayout().getABITypeAlign(llvm::unwrap(*native))));
    }
    auto extension = get_extension(*carriers, type);
    if (extension) {
      function.addParamAttr(Unsigned_32(parameter), *extension);
    }
  }
  auto result_extension = get_extension(*carriers, owner);
  if (!sret && result_extension) {
    function.addRetAttr(*result_extension);
  }

  record.function = llvm::wrap(&function);
  record.completed = True;
  if (record.provider && target->get_unit().is_package_member()) {
    target->add_publication(Publication(owner, record.symbol));
  }
  return True;
}

static auto lower_construction_value(
    Tetrodotoxin::Library::Llvm::Body& body,
    Tetrodotoxin::Library::Llvm::Builder& builder,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Type& type,
    const Ttx::Model::Pack& value) -> Core::Option<LLVMValueRef> {
  auto library = value.select<Tetrodotoxin::Library::Language::Model::Pack>();
  BAIL_IF(!library || !library->lower(builder));
  auto lowered = body.find_values(value);
  BAIL_IF(!lowered);
  return carriers.fit_and_assemble(body, type, value, lowered->get_view());
}

auto Tetrodotoxin::Library::Llvm::Functions::lower_construction(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& owner,
    Core::View::Vector<ConstructionField> fields) const -> Bool {
  auto target = select_program(program);
  auto found = constructions.find(&owner);
  if (!target || !found || !found->value.completed || !found->value.function) {
    return fail_backend(
        program, "LLVM cannot lower Type construction before completion."_view);
  }

  ConstructionRecord& record = found->value;
  if (!record.provider || record.lowered) {
    return True;
  }

  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(*record.function));
  if (!function.empty()) {
    return fail_backend(
        program, "LLVM cannot lower Type construction more than once."_view);
  }
  llvm::BasicBlock::Create(function.getContext(), "entry", &function);
  Core::Option<LLVMValueRef> sret;
  auto argument = function.arg_begin();
  if (record.sret_type) {
    BAIL_IF(argument == function.arg_end());
    sret = llvm::wrap(&*argument);
    argument++;
  }

  Llvm::Body native_body(
      *target, owner, *record.function, {}, sret, record.sret_type);
  llvm::IRBuilder<>& native_builder = get_builder(native_body);
  Llvm::Builder builder(native_body);
  const Carriers& carriers = target->get_carriers();
  Memory::Dynamic::Vector<LLVMValueRef> values;
  Count parameter_index = 0;
  for (const ConstructionField& input : fields) {
    const Ttx::Model::Addressable& field = input.get_field();

    Core::Option<LLVMValueRef> supplied;
    Core::Option<LLVMValueRef> present;
    if (input.is_parameter()) {
      BAIL_IF(
          parameter_index >= record.parameters.get_size() ||
          &record.parameters[parameter_index].get() != &field ||
          argument == function.arg_end());
      llvm::Value& native_value = *argument;
      argument++;
      auto carrier = carriers.get_type(field.get_type());
      BAIL_IF(!carrier);
      supplied =
          record.indirect_parameters[parameter_index]
              ? Core::Option<LLVMValueRef>(llvm::wrap(native_builder.CreateLoad(
                    llvm::unwrap(*carrier), &native_value)))
              : Core::Option<LLVMValueRef>(llvm::wrap(&native_value));
      BAIL_IF(argument == function.arg_end());
      present = llvm::wrap(&*argument);
      argument++;
      parameter_index++;
    }

    if (!supplied || !present) {
      auto lowered = lower_construction_value(
          native_body, builder, carriers, field.get_type(),
          input.get_fallback());
      BAIL_IF(!lowered);
      values.insert(*lowered);
      continue;
    }

    llvm::BasicBlock& supplied_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.supplied", &function);
    llvm::BasicBlock& fallback_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.default", &function);
    llvm::BasicBlock& merge_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.merge", &function);
    native_builder.CreateCondBr(
        llvm::unwrap(*present), &supplied_block, &fallback_block);

    native_builder.SetInsertPoint(&supplied_block);
    native_builder.CreateBr(&merge_block);

    native_builder.SetInsertPoint(&fallback_block);
    auto lowered = lower_construction_value(
        native_body, builder, carriers, field.get_type(), input.get_fallback());
    BAIL_IF(!lowered);
    llvm::BasicBlock* fallback_end = native_builder.GetInsertBlock();
    BAIL_IF(!fallback_end || fallback_end->getTerminator());
    native_builder.CreateBr(&merge_block);

    native_builder.SetInsertPoint(&merge_block);
    llvm::PHINode& selected = *native_builder.CreatePHI(
        llvm::unwrap(*supplied)->getType(), 2, "construction.value");
    selected.addIncoming(llvm::unwrap(*supplied), &supplied_block);
    selected.addIncoming(llvm::unwrap(*lowered), fallback_end);
    values.insert(llvm::wrap(&selected));
  }
  BAIL_IF(
      parameter_index != record.parameters.get_size() ||
      argument != function.arg_end());

  auto constructed = carriers.construct(native_body, owner, values.get_view());
  BAIL_IF(
      !constructed || !native_body.acquire(owner, *constructed) ||
      !native_body.emit_storage_cleanup(0) ||
      !native_body.clear_temporary_cleanup() ||
      !native_body.create_return(*constructed));
  record.lowered = True;
  return True;
}

static auto select_construction_argument(
    const Ttx::Model::Pack& arguments,
    Core::View::Bytes name) -> Core::Option<const Ttx::Model::Pack&> {
  const Ttx::Concept::Layout& layout = arguments.get_layout();
  Core::Option<const Ttx::Model::Pack&> selected;
  for (Count index = 0; index < layout.get_size(); index++) {
    auto candidate_name = layout.get_name(index);
    if (!candidate_name || *candidate_name != name) {
      continue;
    }
    auto produced = arguments.get_produced(index);
    auto pack = produced ? produced->producer.select<Ttx::Model::Pack>()
                         : Core::Option<const Ttx::Model::Pack&>();
    BAIL_IF(selected || !pack);
    selected = *pack;
  }
  return selected;
}

auto Tetrodotoxin::Library::Llvm::Functions::call_construction(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& owner,
    const Ttx::Model::Pack& arguments) const -> Bool {
  auto native_body = body.select<Llvm::Body>();
  auto found = constructions.find(&owner);
  if (!native_body || !found || !found->value.completed ||
      !found->value.function) {
    return fail_backend(
        get_program(body),
        "LLVM cannot call Type construction before completion."_view);
  }

  ConstructionRecord& record = found->value;
  const Carriers& carriers = native_body->get_program().get_carriers();
  Body::NativeValues native_arguments(record.parameters.get_size() * 2 + 1);
  Core::Option<LLVMValueRef> returned_storage;
  if (record.sret_type) {
    returned_storage = native_body->create_entry_alloca(
        *record.sret_type, "construction.result"_view);
    BAIL_IF(!returned_storage);
    native_arguments.insert(*returned_storage);
  }

  for (Count index = 0; index < record.parameters.get_size(); index++) {
    const Ttx::Model::Addressable& field = record.parameters[index].get();
    auto selected = select_construction_argument(arguments, field.get_name());
    Core::Option<LLVMValueRef> native;
    if (selected) {
      auto lowered = native_body->find_values(*selected);
      if (lowered) {
        native = carriers.fit_and_assemble(
            *native_body, field.get_type(), *selected, lowered->get_view());
      }
    } else {
      native = carriers.zero(native_body->get_program(), field.get_type());
    }
    BAIL_IF(!native);

    LLVMValueRef argument = *native;
    if (record.indirect_parameters[index]) {
      auto carrier = carriers.get_type(field.get_type());
      BAIL_IF(!carrier);
      auto storage = native_body->create_entry_alloca(
          *carrier, "construction.argument"_view);
      BAIL_IF(
          !storage ||
          !LLVMBuildStore(native_body->get_builder(), *native, storage));
      argument = storage;
    }
    native_arguments.insert(argument);
    native_arguments.insert(LLVMConstInt(
        LLVMInt1TypeInContext(&native_body->get_program().get_context()),
        selected ? 1 : 0, 0));
  }

  LLVMTypeRef signature = LLVMGlobalGetValueType(*record.function);
  LLVMTypeRef native_result = LLVMGetReturnType(signature);
  Bool returns_void = Bool(LLVMGetTypeKind(native_result) == LLVMVoidTypeKind);
  LLVMValueRef invoked = LLVMBuildCall2(
      native_body->get_builder(), signature, *record.function,
      native_arguments.get_data(), Unsigned_32(native_arguments.get_size()),
      returns_void ? "" : "construction");
  BAIL_IF(!invoked);
  LLVMValueRef returned =
      returned_storage ? LLVMBuildLoad2(
                             native_body->get_builder(), *record.sret_type,
                             *returned_storage, "construction.value")
                       : invoked;
  BAIL_IF(!returned);
  native_body->mark_owned(owner, returned);
  Core::Static::Vector<LLVMValueRef, 1> values = {{returned}};
  return native_body->publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Functions::complete(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable) const -> Bool {
  auto found = records.find(&callable);
  if (!found) {
    return fail_backend(
        program,
        "LLVM cannot complete a Callable before its owner reserves it."_view);
  }

  Record& record = found->value;
  if (record.completed) {
    return True;
  }

  auto target = select_program(program);
  auto carriers = select_carriers(program);
  if (!target || !carriers) {
    return False;
  }

  Core::Option<Core::View::Bytes> abi;
  Core::Option<Core::View::Bytes> symbol;
  Bool c_boundary = Bool(record.kind == Kind::Foreign);
  Bool c_publication = False;
  Bool package_publication = False;
  if ((record.kind == Kind::Function || record.kind == Kind::External) &&
      record.definition) {
    if (!select_function_attributes(program, *record.definition, abi, symbol)) {
      return False;
    }

    c_boundary = Bool(abi);
    if (record.kind == Kind::Function) {
      c_publication = c_boundary;
      package_publication = Bool(
          target->get_unit().is_package_member() &&
          record.definition->is_published() && !c_publication);
    }
  } else if (record.kind == Kind::Function) {
    return fail_backend(
        program, "LLVM Function lowering lost its declaration owner."_view);
  }

  Core::View::Bytes selected_symbol = record.symbol;
  if (record.kind == Kind::Function && symbol) {
    selected_symbol = *symbol;
  } else if (record.kind == Kind::Function) {
    Symbol generated(
        target->get_arena(), callable,
        declares_self(callable) ? Symbol::Kind::FunctionSelf
                                : Symbol::Kind::FunctionStatic,
        target->get_unit());
    selected_symbol = generated.get_view();
  }

  llvm::Module& module = get_module(*target);
  if (module.getNamedValue(llvm_text(selected_symbol))) {
    return fail_callable(
        program, record.definition,
        "The native symbol collides with another emitted declaration."_view,
        "Choose a different `@symbol` spelling or declaration path."_view);
  }

  Memory::Dynamic::Vector<llvm::Type*> parameter_types;
  Memory::Dynamic::Vector<const Ttx::Model::Type*> semantic_parameters;
  Memory::Dynamic::Vector<llvm::Type*> result_types;
  Memory::Dynamic::Vector<const Ttx::Model::Type*> semantic_results;
  if (!select_parameter_types(
          program, *carriers, callable, parameter_types, semantic_parameters)) {
    return False;
  }

  if (!select_result_types(
          program, *carriers, callable, result_types, semantic_results)) {
    return False;
  }

  llvm::LLVMContext& context = get_context(*target);
  llvm::Type* result = llvm::Type::getVoidTy(context);
  if (result_types.get_size() == 1) {
    result = result_types[0];
  } else if (result_types.get_size() != 0) {
    result = llvm::StructType::get(
        context, llvm::ArrayRef<llvm::Type*>(
                     result_types.get_data(), result_types.get_size()));
  }

  Bool sret = Bool(c_boundary && uses_memory_abi(*target, *result));
  Memory::Dynamic::Vector<llvm::Type*> native_parameters;
  if (sret) {
    native_parameters.insert(llvm::PointerType::getUnqual(context));
    record.sret_type = llvm::wrap(result);
  }

  record.indirect_parameters.clear();
  for (Count index = 0; index < parameter_types.get_size(); index++) {
    llvm::Type* parameter = parameter_types[index];
    Bool self_reference = Bool(index == 0 && declares_self(callable));
    Bool indirect = self_reference ||
                    Bool(c_boundary && uses_memory_abi(*target, *parameter));
    record.indirect_parameters.insert(indirect);
    native_parameters.insert(
        indirect ? llvm::PointerType::getUnqual(context) : parameter);
  }

  llvm::FunctionType* signature = llvm::FunctionType::get(
      sret ? llvm::Type::getVoidTy(context) : result,
      llvm::ArrayRef<llvm::Type*>(
          native_parameters.get_data(), native_parameters.get_size()),
      false);
  llvm::GlobalValue::LinkageTypes linkage =
      record.kind == Kind::Foreign || record.kind == Kind::External ||
              c_publication || package_publication
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage;
  llvm::Function& function = *llvm::Function::Create(
      signature, linkage, llvm_text(selected_symbol), module);
  if (record.kind == Kind::External || package_publication) {
    function.setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  Count parameter_offset = sret ? 1 : 0;
  if (sret) {
    function.addParamAttr(
        0, llvm::Attribute::getWithStructRetType(context, result));
    function.addParamAttr(0, llvm::Attribute::NoAlias);
    function.addParamAttr(
        0, llvm::Attribute::getWithAlignment(
               context, module.getDataLayout().getABITypeAlign(result)));
  }

  for (Count index = 0; index < parameter_types.get_size(); index++) {
    Bool self_reference = Bool(index == 0 && declares_self(callable));
    if (!record.indirect_parameters[index] || self_reference) {
      continue;
    }

    function.addParamAttr(
        Unsigned_32(index + parameter_offset),
        llvm::Attribute::getWithByValType(context, parameter_types[index]));
    function.addParamAttr(
        Unsigned_32(index + parameter_offset),
        llvm::Attribute::getWithAlignment(
            context,
            module.getDataLayout().getABITypeAlign(parameter_types[index])));
  }

  for (Count index = 0; index < semantic_parameters.get_size(); index++) {
    auto extension = get_extension(*carriers, *semantic_parameters[index]);
    if (extension) {
      function.addParamAttr(Unsigned_32(index + parameter_offset), *extension);
    }
  }

  auto library_callable = callable.select<Language::Model::Callable>();
  if (semantic_results.get_size() == 1 &&
      !(library_callable && library_callable->get_self_result())) {
    auto extension = get_extension(*carriers, *semantic_results[0]);
    if (extension) {
      function.addRetAttr(*extension);
    }
  }

  record.abi = abi ? *abi : Core::View::Bytes();
  record.symbol = selected_symbol;
  record.function = llvm::wrap(&function);
  record.completed = True;
  if (c_publication || package_publication) {
    target->add_export(Export(callable, selected_symbol));
  }
  if (target->get_unit().is_package_member() &&
      (c_publication || package_publication)) {
    target->add_publication(Publication(callable, selected_symbol));
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Functions::begin_body(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable) const -> Core::Option<Lowering> {
  auto found = records.find(&callable);
  auto target = select_program(program);
  if (!found || !target || found->value.kind != Kind::Function ||
      !found->value.completed || !found->value.function) {
    fail_backend(
        program,
        "LLVM cannot begin a Body for this Callable declaration."_view);
    return {};
  }

  Record& record = found->value;
  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(*record.function));
  if (!function.empty()) {
    fail_callable(
        program, record.definition,
        "LLVM cannot lower one Callable Body more than once."_view);
    return {};
  }

  Core::Option<LLVMValueRef> sret;
  if (record.sret_type) {
    auto argument = function.arg_begin();
    if (argument == function.arg_end()) {
      fail_backend(
          program,
          "LLVM lost the reserved result address for a Callable Body."_view);
      return {};
    }

    sret = llvm::wrap(&*argument);
  }

  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  if (record.indirect_parameters.get_size() != parameters.get_size()) {
    fail_backend(
        program,
        "LLVM Callable parameter state does not match its completed Layout."_view);
    return {};
  }

  llvm::BasicBlock::Create(function.getContext(), "entry", &function);
  return Lowering(*record.function, callable, sret, record.sret_type);
}

auto Tetrodotoxin::Library::Llvm::Functions::bind_parameters(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Callable& callable) const -> Bool {
  auto native_body = body.select<Tetrodotoxin::Library::Llvm::Body>();
  auto found = records.find(&callable);
  if (!native_body || !found || found->value.kind != Kind::Function ||
      !found->value.function ||
      native_body->get_function() != *found->value.function) {
    return False;
  }

  Record& record = found->value;
  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(*record.function));
  if (function.empty()) {
    return fail_backend(
        get_program(body),
        "LLVM cannot bind parameters without a Callable entry block."_view);
  }

  get_builder(*native_body).SetInsertPoint(&function.getEntryBlock());
  get_builder(*native_body).SetCurrentDebugLocation(llvm::DebugLoc());
  auto argument = function.arg_begin();
  if (record.sret_type) {
    argument++;
  }

  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto entry_value = parameters.get_abstract(index);
    auto parameter = entry_value
                         ? entry_value->select<Ttx::Model::Addressable>()
                         : Core::Option<const Ttx::Model::Addressable&>();
    if (!parameter || argument == function.arg_end()) {
      fail_backend(
          get_program(body),
          "LLVM cannot bind the completed Callable parameter Layout."_view);
      return False;
    }

    llvm::Argument& native_argument = *argument;
    argument++;
    LLVMValueRef address = llvm::wrap(&native_argument);
    if (!record.indirect_parameters[index]) {
      LLVMValueRef storage = native_body->create_entry_alloca(
          llvm::wrap(native_argument.getType()), "parameter"_view);
      get_builder(*native_body)
          .CreateStore(&native_argument, llvm::unwrap(storage));
      address = storage;
    }

    Bool published = native_body->publish_address(*parameter, address);
    if (!published) {
      fail_backend(
          get_program(body),
          "LLVM cannot publish one Callable parameter address twice."_view);
      return False;
    }
  }

  if (argument != function.arg_end()) {
    fail_backend(
        get_program(body),
        "LLVM Callable signature retains an unmatched native parameter."_view);
    return False;
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Functions::end_body(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Callable& callable) const -> Bool {
  auto native_body = body.select<Tetrodotoxin::Library::Llvm::Body>();
  auto target =
      get_program(body).select<Tetrodotoxin::Library::Llvm::Program>();
  if (!native_body || !target) {
    return False;
  }

  auto retained_callable = native_body->get_callable();
  if (!retained_callable || &*retained_callable != &callable) {
    return target->fail_backend(
        "LLVM completed a Body under a different Callable identity."_view);
  }

  llvm::BasicBlock* block = get_builder(*native_body).GetInsertBlock();
  Bool completed = True;
  if (!block) {
    completed = target->fail_backend(
        "LLVM completed a Callable Body without an insertion block."_view);
  } else if (!block->getTerminator()) {
    auto library_callable = callable.select<Language::Model::Callable>();
    auto self_result = library_callable
                           ? library_callable->get_self_result()
                           : Core::Option<const Ttx::Model::Addressable&>();
    if (self_result) {
      auto address = native_body->find_address(*self_result);
      completed = Bool(
          address && llvm::unwrap(*address)->getType() ==
                         block->getParent()->getReturnType());
      if (completed) {
        completed = native_body->emit_storage_cleanup(0);
        completed &= native_body->emit_temporary_cleanup();
        completed &= native_body->create_return(*address);
      } else {
        completed = target->fail_backend(
            "LLVM could not return the fallthrough Self reference."_view);
      }
    } else if (callable.get_results().is_empty()) {
      completed = native_body->emit_storage_cleanup(0);
      completed &= native_body->emit_temporary_cleanup();
      completed &= native_body->create_return();
    } else {
      auto found = records.find(&callable);
      completed = fail_callable(
          get_program(body),
          found ? found->value.definition
                : Core::Option<const Tetrodotoxin::Language::Definition&>(),
          "A value returning Callable reached the end of its Body."_view,
          "Return the complete declared result Layout on every reachable path."_view);
    }
  }

  return completed;
}

auto Tetrodotoxin::Library::Llvm::Functions::find_function(
    const Ttx::Model::Callable& callable) const -> Core::Option<LLVMValueRef> {
  auto found = records.find(&callable);
  return found ? found->value.function : Core::Option<LLVMValueRef>();
}

auto Tetrodotoxin::Library::Llvm::Functions::find_symbol(
    const Ttx::Model::Callable& callable) const
    -> Core::Option<Core::View::Bytes> {
  auto found = records.find(&callable);
  return found && found->value.completed
             ? Core::Option<Core::View::Bytes>(found->value.symbol)
             : Core::Option<Core::View::Bytes>();
}

auto Tetrodotoxin::Library::Llvm::Functions::find_sret_type(
    const Ttx::Model::Callable& callable) const -> Core::Option<LLVMTypeRef> {
  auto found = records.find(&callable);
  return found ? found->value.sret_type : Core::Option<LLVMTypeRef>();
}

auto Tetrodotoxin::Library::Llvm::Functions::get_indirect_parameters(
    const Ttx::Model::Callable& callable) const -> Core::View::Vector<Bool> {
  auto found = records.find(&callable);
  return found ? found->value.indirect_parameters.get_view()
               : Core::View::Vector<Bool>();
}

auto Tetrodotoxin::Library::Llvm::Functions::get_foreign_callables() const
    -> Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Callable>> {
  return foreign_callables.get_view();
}
