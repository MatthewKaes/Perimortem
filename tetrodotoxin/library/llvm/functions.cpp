// Perimortem Engine
// Copyright © Matt Kaes

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/Function.h")
#include "llvm/IR/Function.h"
#else
#error LLVM Function is required by the Library native compiler
#endif

#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "tetrodotoxin/library/language/model/callable.hpp"
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

  auto reserved =
      reserve(program, callable, Record(Kind::Foreign, abi, symbol));
  if (reserved && *reserved) {
    foreign_callables.insert(callable);
  }

  return reserved;
}

auto Tetrodotoxin::Library::Llvm::Functions::reserve_construction(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Callable& callable,
    const Ttx::Model::Type& owner) const -> Core::Option<Bool> {
  auto target = select_program(program);
  BAIL_IF(!target);

  auto external = target->get_unit().find(callable);
  if (target->get_unit().is_package_member() && external) {
    return reserve(
        program, callable, Record(Kind::External, {}, *external, {}, True));
  }

  Symbol symbol(
      target->get_arena(), owner, Symbol::Kind::Construction,
      target->get_unit());
  return reserve(
      program, callable,
      Record(Kind::Function, {}, symbol.get_view(), {}, True));
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
  } else if (record.kind == Kind::Function && record.construction) {
    package_publication = target->get_unit().is_package_member();
  } else if (record.kind == Kind::Function) {
    return fail_backend(
        program, "LLVM Function lowering lost its declaration owner."_view);
  }

  Core::View::Bytes selected_symbol = record.symbol;
  if (record.kind == Kind::Function && symbol) {
    selected_symbol = *symbol;
  } else if (record.kind == Kind::Function && !record.construction) {
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
