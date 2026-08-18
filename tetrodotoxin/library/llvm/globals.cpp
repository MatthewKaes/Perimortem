// Perimortem Engine
// Copyright © Matt Kaes

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
// clang-format off
#include "llvm/IR/GlobalVariable.h"
#include "tetrodotoxin/library/llvm/globals.hpp"
// clang-format on

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "perimortem/abi/memory/dynamic/object.hpp"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "tetrodotoxin/library/llvm/carriers.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/library/llvm/symbol.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto get_target(Ttx::Concept::Abstract& program)
    -> Core::Option<Tetrodotoxin::Library::Llvm::Program&> {
  return program.select<Tetrodotoxin::Library::Llvm::Program>();
}

static auto get_program(Ttx::Concept::Abstract& body)
    -> Ttx::Concept::Abstract& {
  auto selected = body.select<Tetrodotoxin::Library::Llvm::Body>();
  return selected ? selected->get_program() : body;
}

static auto get_carriers(Ttx::Concept::Abstract& program)
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
  auto target = get_target(program);
  return target ? target->fail_backend(message) : False;
}

static auto create_void_function(Tetrodotoxin::Library::Llvm::Program& program)
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
    Tetrodotoxin::Library::Llvm::Program& program,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Addressable& addressable,
    LLVMValueRef global) -> Core::Option<LLVMValueRef> {
  LLVMValueRef function = create_void_function(program);
  auto& native_function = *llvm::unwrap<llvm::Function>(function);
  llvm::BasicBlock::Create(
      native_function.getContext(), "entry", &native_function);
  Tetrodotoxin::Library::Llvm::Body body(program, addressable, function);
  llvm::IRBuilder<>& builder = get_builder(body);
  auto native_type = carriers.get_type(addressable.get_type());
  if (!native_type) {
    return {};
  }

  LLVMValueRef value = llvm::wrap(
      builder.CreateLoad(llvm::unwrap(*native_type), llvm::unwrap(global)));
  Bool released = carriers.release(body, addressable.get_type(), value);
  Bool returned = released && body.create_return();
  return returned ? Core::Option<LLVMValueRef>(function)
                  : Core::Option<LLVMValueRef>();
}

static auto register_destructor(
    Tetrodotoxin::Library::Llvm::Program& program,
    Tetrodotoxin::Library::Llvm::Body& body,
    LLVMValueRef destructor) -> Bool {
  llvm::LLVMContext& context = get_context(program);
  llvm::Module& module = get_module(program);
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::Type::getVoidTy(context), {llvm::PointerType::getUnqual(context)},
      false);
  llvm::FunctionCallee registration = module.getOrInsertFunction(
      llvm_text(Abi::Memory::Dynamic::Object::register_cleanup_symbol),
      &signature);

  get_builder(body).CreateCall(registration, {llvm::unwrap(destructor)});
  return True;
}

auto Tetrodotoxin::Library::Llvm::Globals::reserve(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Addressable& addressable,
    Record record) const -> Core::Option<Bool> {
  auto found = records.find(&addressable);
  if (found) {
    Bool same = Bool(
        found->value.foreign == record.foreign &&
        found->value.abi == record.abi &&
        found->value.symbol == record.symbol &&
        found->value.writable == record.writable);
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

auto Tetrodotoxin::Library::Llvm::Globals::reserve_static(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Addressable& addressable) const -> Core::Option<Bool> {
  auto target = get_target(program);
  BAIL_IF(!target);

  Symbol symbol(target->get_arena(), addressable, Symbol::Kind::Address);
  return reserve(
      program, addressable, Record(False, {}, symbol.get_view(), True));
}

auto Tetrodotoxin::Library::Llvm::Globals::reserve_foreign(
    Ttx::Concept::Abstract& program,
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
  }

  return reserved;
}

auto Tetrodotoxin::Library::Llvm::Globals::complete(
    Ttx::Concept::Abstract& program,
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
  if (record.completed) {
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

  global->setConstant(bool(record.foreign && !record.writable));
  global->setLinkage(
      record.foreign ? llvm::GlobalValue::ExternalLinkage
                     : llvm::GlobalValue::InternalLinkage);
  if (!record.foreign) {
    global->setInitializer(
        llvm::Constant::getNullValue(llvm::unwrap(*native_type)));
  }

  record.global = llvm::wrap(global);
  record.completed = True;
  return True;
}

auto Tetrodotoxin::Library::Llvm::Globals::begin_initializer(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  auto target = get_target(program);
  if (!found || !target || found->value.foreign || !found->value.completed ||
      !found->value.global || found->value.initializer_function ||
      found->value.initialized) {
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

auto Tetrodotoxin::Library::Llvm::Globals::end_initializer(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Addressable& addressable,
    const Ttx::Model::Pack& value) const -> Bool {
  auto native_body = body.select<Tetrodotoxin::Library::Llvm::Body>();
  auto target =
      get_program(body).select<Tetrodotoxin::Library::Llvm::Program>();
  auto carriers = get_carriers(get_program(body));
  auto found = records.find(&addressable);
  if (!native_body || !target || !carriers || !found ||
      &native_body->get_owner() != &addressable || !found->value.global ||
      !found->value.initializer_function || found->value.initialized ||
      native_body->get_function() != *found->value.initializer_function) {
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
    found->value.initialized = True;
  }

  return completed;
}

auto Tetrodotoxin::Library::Llvm::Globals::find_address(
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  return found ? found->value.global : Core::Option<LLVMValueRef>();
}

auto Tetrodotoxin::Library::Llvm::Globals::find_symbol(
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<Core::View::Bytes> {
  auto found = records.find(&addressable);
  return found && found->value.completed
             ? Core::Option<Core::View::Bytes>(found->value.symbol)
             : Core::Option<Core::View::Bytes>();
}

auto Tetrodotoxin::Library::Llvm::Globals::permits_foreign_write(
    const Ttx::Model::Addressable& addressable) const -> Bool {
  auto found = records.find(&addressable);
  return found && found->value.foreign && found->value.writable;
}

auto Tetrodotoxin::Library::Llvm::Globals::get_foreign_addressables() const
    -> Core::View::Vector<
        Ttx::Concept::Reference<const Ttx::Model::Addressable>> {
  return foreign_addressables.get_view();
}
