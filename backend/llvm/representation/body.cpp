// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the native backend
#endif
#include "backend/llvm/representation/body.hpp"
#include "backend/llvm/representation/carriers.hpp"
#include "backend/llvm/representation/program.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

using namespace Perimortem;
using namespace Tetrodotoxin::Backend;

static auto select_carriers(const Llvm::Representation::Body& body)
    -> Core::Option<const Llvm::Representation::Carriers&> {
  return body.get_program().get_carriers();
}

static auto native_builder(const Llvm::Representation::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

Llvm::Representation::Body::Body(
    Llvm::Representation::Program& program,
    const Ttx::Concept::Abstract& owner,
    LLVMValueRef function,
    Core::Option<const Ttx::Model::Callable&> callable,
    Core::Option<LLVMValueRef> sret,
    Core::Option<LLVMTypeRef> sret_type)
    : Emission(Emission::Kind::Body),
      program(program),
      owner(owner),
      function(function),
      callable(callable),
      sret(sret),
      sret_type(sret_type),
      builder(
          llvm::wrap(new llvm::IRBuilder<>(
              llvm::unwrap<llvm::Function>(function)->getContext()))) {
  llvm::Function& native = *llvm::unwrap<llvm::Function>(function);
  if (!native.empty()) {
    native_builder(*this).SetInsertPoint(&native.getEntryBlock());
  }

  native_builder(*this).SetCurrentDebugLocation(llvm::DebugLoc());
}

Llvm::Representation::Body::~Body() {
  delete reinterpret_cast<llvm::IRBuilder<>*>(builder);
}

auto Llvm::Representation::Body::get_builder() const -> LLVMBuilderRef {
  return builder;
}

auto Llvm::Representation::Body::get_function() const -> LLVMValueRef {
  return function;
}

auto Llvm::Representation::Body::get_callable() const
    -> Core::Option<const Ttx::Model::Callable&> {
  return callable;
}

auto Llvm::Representation::Body::get_sret() const
    -> Core::Option<LLVMValueRef> {
  return sret;
}

auto Llvm::Representation::Body::get_sret_type() const
    -> Core::Option<LLVMTypeRef> {
  return sret_type;
}

auto Llvm::Representation::Body::find_values(const Ttx::Model::Pack& pack) const
    -> Core::Option<const NativeValues&> {
  auto found = values.find(&pack);
  return found ? Core::Option<const NativeValues&>(found->value)
               : Core::Option<const NativeValues&>();
}

auto Llvm::Representation::Body::find_value(const Ttx::Model::Pack& pack) const
    -> Core::Option<LLVMValueRef> {
  auto found = find_values(pack);
  if (!found || found->get_size() != 1) {
    return {};
  }

  LLVMValueRef value = found->get_data()[0];
  return value ? Core::Option<LLVMValueRef>(value)
               : Core::Option<LLVMValueRef>();
}

auto Llvm::Representation::Body::publish_values(
    const Ttx::Model::Pack& pack,
    Core::View::Vector<LLVMValueRef> native) -> Bool {
  NativeValues retained(native.get_size());
  for (LLVMValueRef value : native) {
    if (!value) {
      return False;
    }

    retained.insert(value);
  }

  values.insert(&pack, retained);
  return True;
}

auto Llvm::Representation::Body::find_address(
    const Ttx::Model::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = addresses.find(&addressable);
  return found ? Core::Option<LLVMValueRef>(found->value)
               : Core::Option<LLVMValueRef>();
}

auto Llvm::Representation::Body::publish_address(
    const Ttx::Model::Addressable& addressable,
    LLVMValueRef value) -> Bool {
  if (!value) {
    return False;
  }

  addresses.insert(&addressable, value);
  return True;
}

auto Llvm::Representation::Body::find_target_address(
    const Ttx::Model::Pack& pack) const -> Core::Option<const TargetAddress&> {
  auto found = target_addresses.find(&pack);
  return found ? Core::Option<const TargetAddress&>(found->value)
               : Core::Option<const TargetAddress&>();
}

auto Llvm::Representation::Body::publish_target_address(
    const Ttx::Model::Pack& pack,
    const Ttx::Model::Type& type,
    LLVMValueRef address) -> Bool {
  if (target_addresses.contains(&pack) || !address) {
    return False;
  }

  target_addresses.insert(&pack, TargetAddress(type, address));
  return True;
}

auto Llvm::Representation::Body::find_indexed_target(
    const Ttx::Model::Pack& pack) const -> Core::Option<const IndexedTarget&> {
  auto found = indexed_targets.find(&pack);
  return found ? Core::Option<const IndexedTarget&>(found->value)
               : Core::Option<const IndexedTarget&>();
}

auto Llvm::Representation::Body::publish_indexed_target(
    const Ttx::Model::Pack& pack,
    const Ttx::Model::Type& type,
    LLVMTypeRef native_type,
    LLVMValueRef data,
    LLVMValueRef length,
    LLVMValueRef first,
    Core::Option<Count> range_size) -> Bool {
  if (indexed_targets.contains(&pack) || !native_type || !data || !length ||
      !first) {
    return False;
  }

  indexed_targets.insert(
      &pack, IndexedTarget(type, native_type, data, length, first, range_size));
  return True;
}

auto Llvm::Representation::Body::find_selection(
    const Ttx::Model::Pack& pack) const -> Core::Option<LLVMValueRef> {
  auto found = selections.find(&pack);
  return found ? Core::Option<LLVMValueRef>(found->value)
               : Core::Option<LLVMValueRef>();
}

auto Llvm::Representation::Body::publish_selection(
    const Ttx::Model::Pack& pack,
    LLVMValueRef value) -> Bool {
  if (!value) {
    return False;
  }

  selections.insert(&pack, value);
  return True;
}

auto Llvm::Representation::Body::get_storage_depth() const -> Count {
  return owned_storages.get_size();
}

auto Llvm::Representation::Body::register_storage(
    const Ttx::Model::Type& type,
    LLVMValueRef address) -> Bool {
  auto carriers = select_carriers(*this);
  if (!carriers || !address) {
    return False;
  }

  if (carriers->owns_resources(type)) {
    owned_storages.insert(OwnedStorage{type, address});
  }

  return True;
}

auto Llvm::Representation::Body::resize_storage(Count size) -> Bool {
  if (size > owned_storages.get_size()) {
    return False;
  }

  while (owned_storages.get_size() != size) {
    owned_storages.remove(owned_storages.get_size() - 1);
  }

  return True;
}

auto Llvm::Representation::Body::mark_owned(
    const Ttx::Model::Type& type,
    LLVMValueRef value) -> void {
  auto carriers = select_carriers(*this);
  if (!carriers || !value || !carriers->owns_resources(type)) {
    return;
  }

  for (const OwnedValue& owned : owned_values.get_view()) {
    if (owned.value == value) {
      return;
    }
  }

  owned_values.insert(OwnedValue{type, value});
}

auto Llvm::Representation::Body::take_owned(LLVMValueRef value) -> Bool {
  for (Count index = 0; index < owned_values.get_size(); index++) {
    if (owned_values[index].value == value) {
      return owned_values.remove_stable(index);
    }
  }

  return False;
}

auto Llvm::Representation::Body::acquire(
    const Ttx::Model::Type& type,
    LLVMValueRef value) -> Bool {
  if (!value) {
    return False;
  }

  if (take_owned(value)) {
    return True;
  }

  auto carriers = select_carriers(*this);
  return carriers && carriers->retain(*this, type, value);
}

auto Llvm::Representation::Body::emit_storage_cleanup(Count first) -> Bool {
  auto carriers = select_carriers(*this);
  if (!carriers || first > owned_storages.get_size()) {
    return False;
  }

  llvm::IRBuilder<>& selected_builder = native_builder(*this);
  for (Count index = owned_storages.get_size(); index != first; index--) {
    const OwnedStorage& owned = owned_storages[index - 1];
    auto native = carriers->get_type(owned.type.get());
    if (!native) {
      return False;
    }

    LLVMValueRef value = llvm::wrap(selected_builder.CreateLoad(
        llvm::unwrap(*native), llvm::unwrap(owned.address)));
    if (!carriers->release(*this, owned.type.get(), value)) {
      return False;
    }
  }

  return True;
}

auto Llvm::Representation::Body::emit_temporary_cleanup() -> Bool {
  llvm::IRBuilder<>& selected_builder = native_builder(*this);
  llvm::BasicBlock* block = selected_builder.GetInsertBlock();
  if (!block || block->getTerminator()) {
    return True;
  }

  auto carriers = select_carriers(*this);
  if (!carriers) {
    return False;
  }

  for (Count index = owned_values.get_size(); index != 0; index--) {
    const OwnedValue& owned = owned_values[index - 1];
    if (!carriers->release(*this, owned.type.get(), owned.value)) {
      return False;
    }
  }

  return True;
}

auto Llvm::Representation::Body::clear_temporary_cleanup() -> Bool {
  Bool emitted = emit_temporary_cleanup();
  owned_values.clear();
  return emitted;
}

auto Llvm::Representation::Body::create_return(Core::Option<LLVMValueRef> value)
    -> Bool {
  llvm::IRBuilder<>& selected_builder = native_builder(*this);
  if (sret) {
    if (!value) {
      return False;
    }

    selected_builder.CreateStore(llvm::unwrap(*value), llvm::unwrap(*sret));
    selected_builder.CreateRetVoid();
    return True;
  }

  if (value) {
    selected_builder.CreateRet(llvm::unwrap(*value));
  } else {
    selected_builder.CreateRetVoid();
  }

  return True;
}

auto Llvm::Representation::Body::push_block_scope(
    const Ttx::Concept::Abstract& owner) -> Bool {
  block_scopes.insert(BlockScope(owner, get_storage_depth()));
  return True;
}

auto Llvm::Representation::Body::take_block_scope(
    const Ttx::Concept::Abstract& owner) -> Core::Option<BlockScope> {
  if (block_scopes.get_size() == 0) {
    return {};
  }

  Count index = block_scopes.get_size() - 1;
  if (&block_scopes[index].get_owner() != &owner) {
    return {};
  }

  BlockScope selected = block_scopes[index];
  block_scopes.remove(index);
  return selected;
}

auto Llvm::Representation::Body::publish_loop(
    const Ttx::Concept::Abstract& owner,
    LLVMBasicBlockRef break_target,
    LLVMBasicBlockRef continue_target) -> Bool {
  if (find_loop(owner) || !break_target || !continue_target) {
    return False;
  }

  loops.insert(
      LoopTargets(owner, break_target, continue_target, get_storage_depth()));
  return True;
}

auto Llvm::Representation::Body::find_loop(const Ttx::Concept::Abstract& owner)
    const -> Core::Option<const LoopTargets&> {
  for (Count index = loops.get_size(); index != 0; index--) {
    const LoopTargets& selected = loops[index - 1];
    if (&selected.get_owner() == &owner) {
      return selected;
    }
  }

  return {};
}

auto Llvm::Representation::Body::remove_loop(
    const Ttx::Concept::Abstract& owner) -> Bool {
  for (Count index = loops.get_size(); index != 0; index--) {
    if (&loops[index - 1].get_owner() == &owner) {
      return loops.remove_stable(index - 1);
    }
  }

  return False;
}

auto Llvm::Representation::Body::create_entry_alloca(
    LLVMTypeRef type,
    Core::View::Bytes name) -> LLVMValueRef {
  llvm::Function& selected = *llvm::unwrap<llvm::Function>(function);
  llvm::IRBuilder<> entry(
      &selected.getEntryBlock(), selected.getEntryBlock().begin());
  llvm::AllocaInst& address = *entry.CreateAlloca(
      llvm::unwrap(type), nullptr,
      llvm::StringRef(
          reinterpret_cast<const char*>(name.get_data()), name.get_size()));
  return llvm::wrap(&address);
}

auto Llvm::Representation::Body::get_debug_scope() const
    -> Core::Option<LLVMMetadataRef> {
  return debug_scope;
}

auto Llvm::Representation::Body::set_debug_scope(
    Core::Option<LLVMMetadataRef> scope) -> void {
  debug_scope = scope;
}

auto Llvm::Representation::Body::push_debug_scope(LLVMMetadataRef scope)
    -> Bool {
  if (!debug_scope || !scope) {
    return False;
  }

  debug_scopes.insert(*debug_scope);
  debug_scope = scope;
  return True;
}

auto Llvm::Representation::Body::pop_debug_scope() -> Bool {
  if (debug_scopes.get_size() == 0) {
    return False;
  }

  Count index = debug_scopes.get_size() - 1;
  debug_scope = debug_scopes[index];
  debug_scopes.remove(index);
  return True;
}
