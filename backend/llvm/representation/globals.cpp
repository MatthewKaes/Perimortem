// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if defined(__cplusplus)
#include "llvm/IR/GlobalVariable.h"
#endif
#include "backend/llvm/abi/symbol.hpp"
#include "backend/llvm/representation/body.hpp"
#include "backend/llvm/representation/carriers.hpp"
#include "backend/llvm/representation/globals.hpp"
#include "backend/llvm/representation/program.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "perimortem/abi/core/cleanup.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Backend;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto get_target(Llvm::Representation::Emission& program)
    -> Core::Option<Llvm::Representation::Program&> {
  if (program.get_kind() == Llvm::Representation::Emission::Kind::Module) {
    return static_cast<Llvm::Representation::Program&>(program);
  }
  return static_cast<Llvm::Representation::Body&>(program).get_program();
}

static auto is_local_definition(
    const Llvm::Abi::Unit& unit,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> current(
      definition.get_host());
  while (true) {
    auto source =
        current.get().select<Tetrodotoxin::Library::Language::Types::Source>();
    if (source) {
      return unit.owns(source->get_host());
    }
    auto composite =
        current.get()
            .select<Tetrodotoxin::Library::Language::Types::Composite>();
    BAIL_IF(!composite);
    current = composite->get_definition().get_host();
  }
}

static auto get_program(Llvm::Representation::Emission& body)
    -> Llvm::Representation::Program& {
  return body.get_kind() == Llvm::Representation::Emission::Kind::Body
             ? static_cast<Llvm::Representation::Body&>(body).get_program()
             : static_cast<Llvm::Representation::Program&>(body);
}

static auto get_carriers(Llvm::Representation::Emission& program)
    -> Core::Option<const Llvm::Representation::Carriers&> {
  auto native = get_target(program);
  return native ? Core::Option<const Llvm::Representation::Carriers&>(
                      native->get_carriers())
                : Core::Option<const Llvm::Representation::Carriers&>();
}

static auto get_context(Llvm::Representation::Program& program)
    -> llvm::LLVMContext& {
  return *llvm::unwrap(&program.get_context());
}

static auto get_module(Llvm::Representation::Program& program)
    -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto get_builder(Llvm::Representation::Body& body)
    -> llvm::IRBuilder<>& {
  return *llvm::unwrap(body.get_builder());
}

static auto fail_backend(
    Llvm::Representation::Emission& program,
    Core::View::Bytes message) -> Bool {
  auto target = get_target(program);
  return target ? target->fail_backend(message) : False;
}

static auto create_void_function(Llvm::Representation::Program& program)
    -> LLVMValueRef {
  llvm::LLVMContext& context = get_context(program);
  llvm::Module& module = get_module(program);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
  llvm::Function& function = *llvm::Function::Create(
      &signature, llvm::GlobalValue::InternalLinkage, "", module);
  return llvm::wrap(&function);
}

static auto emit_destructor(
    Llvm::Representation::Program& program,
    const Llvm::Representation::Carriers& carriers,
    const Ttx::Model::Addressable& addressable,
    LLVMValueRef global) -> Core::Option<LLVMValueRef> {
  LLVMValueRef function = create_void_function(program);
  auto& native_function = *llvm::unwrap<llvm::Function>(function);
  llvm::BasicBlock::Create(
      native_function.getContext(), "entry", &native_function);
  Llvm::Representation::Body body(program, addressable, function);
  llvm::IRBuilder<>& builder = get_builder(body);
  auto native_type = carriers.get_type(addressable.get_type());
  if (!native_type) {
    return {};
  }

  LLVMValueRef value = llvm::wrap(
      builder.CreateLoad(llvm::unwrap(*native_type), llvm::unwrap(global)));
  return carriers.release(body, addressable.get_type(), value) &&
                 body.create_return()
             ? Core::Option<LLVMValueRef>(function)
             : Core::Option<LLVMValueRef>();
}

static auto register_destructor(
    Llvm::Representation::Program& program,
    Llvm::Representation::Body& body,
    LLVMValueRef destructor) -> Bool {
  llvm::LLVMContext& context = get_context(program);
  llvm::Module& module = get_module(program);
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::Type::getVoidTy(context), {llvm::PointerType::getUnqual(context)},
      false);
  llvm::FunctionCallee registration = module.getOrInsertFunction(
      llvm_text(Perimortem::Abi::Core::cleanup_register_symbol), &signature);

  get_builder(body).CreateCall(registration, {llvm::unwrap(destructor)});
  return True;
}

auto Llvm::Representation::Globals::reserve(
    Llvm::Representation::Emission& program,
    const Ttx::Model::Addressable& addressable,
    Record record) const -> Core::Option<Bool> {
  auto found = records.find(&addressable);
  if (found) {
    Bool same = Bool(
        found->value.has(Record::Property::Foreign) ==
            record.has(Record::Property::Foreign) &&
        found->value.abi == record.abi &&
        found->value.symbol == record.symbol &&
        found->value.has(Record::Property::Writable) ==
            record.has(Record::Property::Writable) &&
        found->value.has(Record::Property::External) ==
            record.has(Record::Property::External) &&
        found->value.has(Record::Property::Published) ==
            record.has(Record::Property::Published));
    if (!same) {
      fail_backend(
          program,
          "LLVM received different owners for one Addressable identity."_view);
      return {};
    }

    return False;
  }

  records.insert(&addressable, record);
  return True;
}

auto Llvm::Representation::Globals::reserve_static(
    Llvm::Representation::Emission& program,
    const Ttx::Model::Addressable& addressable) const -> Core::Option<Bool> {
  auto target = get_target(program);
  BAIL_IF(!target);

  auto field = addressable.select<Tetrodotoxin::Library::Language::Field>();
  Bool local = !field ||
               is_local_definition(target->get_unit(), field->get_definition());
  if (target->get_unit().is_package_member() && !local) {
    auto symbol = target->get_unit().find(addressable);
    BAIL_IF(!symbol);
    return reserve(
        program, addressable, Record(False, {}, *symbol, True, True, False));
  }

  Llvm::Abi::Symbol symbol(
      target->get_arena(), addressable, Llvm::Abi::Symbol::Kind::Address,
      target->get_unit());
  Bool published = Bool(
      field && target->get_unit().is_package_member() &&
      field->get_definition().is_published());
  return reserve(
      program, addressable,
      Record(False, {}, symbol.get_view(), True, False, published));
}

auto Llvm::Representation::Globals::reserve_foreign(
    Llvm::Representation::Emission& program,
    const Ttx::Model::Addressable& addressable,
    Core::View::Bytes abi,
    Core::View::Bytes symbol,
    Bool writable) const -> Core::Option<Bool> {
  if (abi != "C"_view || symbol.is_empty()) {
    fail_backend(
        program, "LLVM requires one named Foreign C State declaration."_view);
    return {};
  }

  auto reserved =
      reserve(program, addressable, Record(True, abi, symbol, writable));
  if (reserved && *reserved) {
    foreign_addressables.insert(addressable);
    auto target = get_target(program);
    if (!target ||
        !target->add_import(
            Tetrodotoxin::Linker::Import(
                writable ? Tetrodotoxin::Linker::Import::Kind::WritableState
                         : Tetrodotoxin::Linker::Import::Kind::ReadOnlyState,
                abi, symbol))) {
      return {};
    }
  }

  return reserved;
}

auto Llvm::Representation::Globals::complete(
    Llvm::Representation::Emission& program,
    const Ttx::Model::Addressable& addressable) const -> Bool {
  auto found = records.find(&addressable);
  auto target = get_target(program);
  auto carriers = get_carriers(program);
  if (!found || !target || !carriers) {
    return fail_backend(
        program,
        "LLVM cannot complete an Addressable before its owner reserves it."_view);
  }

  Record& record = found->value;
  if (record.global) {
    return True;
  }

  auto native_type = carriers->get_type(addressable.get_type());
  if (!native_type) {
    return fail_backend(
        program,
        "LLVM cannot find the completed carrier for an Addressable."_view);
  }

  llvm::Module& module = get_module(*target);
  if (module.getNamedValue(llvm_text(record.symbol))) {
    return fail_backend(
        program,
        "The native State symbol collides with another emitted declaration."_view);
  }

  llvm::Constant* inserted = module.getOrInsertGlobal(
      llvm_text(record.symbol), llvm::unwrap(*native_type));
  auto global = llvm::dyn_cast<llvm::GlobalVariable>(inserted);
  if (!global) {
    return fail_backend(
        program,
        "LLVM could not publish the completed Addressable carrier."_view);
  }

  global->setConstant(
      bool(
          record.has(Record::Property::Foreign) &&
          !record.has(Record::Property::Writable)));
  global->setLinkage(
      record.has(Record::Property::Foreign) ||
              record.has(Record::Property::External) ||
              record.has(Record::Property::Published)
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage);
  if (record.has(Record::Property::External) ||
      record.has(Record::Property::Published)) {
    global->setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  if (!record.has(Record::Property::Foreign) &&
      !record.has(Record::Property::External)) {
    global->setInitializer(
        llvm::Constant::getNullValue(llvm::unwrap(*native_type)));
  }

  record.global = llvm::wrap(global);
  if (record.has(Record::Property::Published)) {
    target->add_publication(Llvm::Abi::Publication(addressable, record.symbol));
  }
  return True;
}

auto Llvm::Representation::Globals::begin_initializer(
    Llvm::Representation::Emission& program,
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  auto target = get_target(program);
  if (!found || !target || found->value.has(Record::Property::Foreign) ||
      found->value.has(Record::Property::External) || !found->value.global ||
      found->value.initializer_function) {
    fail_backend(
        program, "LLVM cannot begin this Static initializer Body."_view);
    return {};
  }

  LLVMValueRef function = create_void_function(*target);
  auto& native_function = *llvm::unwrap<llvm::Function>(function);
  llvm::BasicBlock::Create(
      native_function.getContext(), "entry", &native_function);
  found->value.initializer_function = function;
  return function;
}

auto Llvm::Representation::Globals::end_initializer(
    Llvm::Representation::Emission& body,
    const Ttx::Model::Addressable& addressable,
    const Ttx::Model::Pack& value) const -> Bool {
  auto native_body =
      body.get_kind() == Llvm::Representation::Emission::Kind::Body
          ? Core::Option<Llvm::Representation::Body&>(
                static_cast<Llvm::Representation::Body&>(body))
          : Core::Option<Llvm::Representation::Body&>();
  auto target = get_target(get_program(body));
  auto carriers = get_carriers(get_program(body));
  auto found = records.find(&addressable);
  if (!native_body || !target || !carriers || !found ||
      &native_body->get_owner() != &addressable || !found->value.global ||
      !found->value.initializer_function ||
      native_body->get_function() != *found->value.initializer_function ||
      (get_builder(*native_body).GetInsertBlock() &&
       get_builder(*native_body).GetInsertBlock()->getTerminator())) {
    return fail_backend(
        get_program(body),
        "LLVM completed a Static initializer under different target state."_view);
  }

  auto values = native_body->find_values(value);
  auto assembled =
      values ? carriers->fit_and_assemble(
                   body, addressable.get_type(), value, values->get_view())
             : Core::Option<LLVMValueRef>();
  Bool completed = Bool(assembled);
  if (completed) {
    completed = native_body->acquire(addressable.get_type(), *assembled);
  }

  if (completed) {
    get_builder(*native_body)
        .CreateStore(
            llvm::unwrap(*assembled), llvm::unwrap(*found->value.global));
    completed = native_body->clear_temporary_cleanup();
  }

  if (completed && carriers->owns_resources(addressable.get_type())) {
    auto destructor =
        emit_destructor(*target, *carriers, addressable, *found->value.global);
    completed =
        destructor && register_destructor(*target, *native_body, *destructor);
  }

  if (completed) {
    completed = native_body->create_return();
  }

  if (completed) {
    auto function =
        llvm::unwrap<llvm::Function>(*found->value.initializer_function);
    llvm::appendToGlobalCtors(get_module(*target), function, 65535);
  }

  return completed;
}

auto Llvm::Representation::Globals::find_address(
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  return found ? found->value.global : Core::Option<LLVMValueRef>();
}

auto Llvm::Representation::Globals::find_symbol(
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<Core::View::Bytes> {
  auto found = records.find(&addressable);
  return found && found->value.global
             ? Core::Option<Core::View::Bytes>(found->value.symbol)
             : Core::Option<Core::View::Bytes>();
}

auto Llvm::Representation::Globals::permits_foreign_write(
    const Ttx::Model::Addressable& addressable) const -> Bool {
  auto found = records.find(&addressable);
  return found && found->value.has(Record::Property::Foreign) &&
         found->value.has(Record::Property::Writable);
}

auto Llvm::Representation::Globals::get_foreign_addressables() const
    -> Core::View::Vector<
        Ttx::Concept::Reference<const Ttx::Model::Addressable>> {
  return foreign_addressables.get_view();
}
