// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "ttx/bootstrap/concept/layout.hpp"
#include "ttx/bootstrap/model/addressable.hpp"
#include "ttx/bootstrap/model/callable.hpp"
#include "ttx/bootstrap/model/type.hpp"
#include "ttx/lexical/anchor.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

// Invocation owns callable ABI, Object operations, borrowed views, and
// aggregate construction for one native Body.
class Invocation {
 public:
  constexpr Invocation(Module::Body& body) : body(body) {}
  Invocation(const Invocation&) = delete;
  Invocation(Invocation&&) = delete;
  auto operator=(const Invocation&) -> Invocation& = delete;
  auto operator=(Invocation&&) -> Invocation& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto invoke(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Pack&> receiver_source)
      const -> Bool;
  auto fit_input(
      const Ttx::Model::Addressable& parameter,
      const Tetrodotoxin::Library::Language::Model::Pack& source,
      Count offset,
      Count size) const -> Perimortem::Core::Option<LLVMValueRef>;
  auto get_size(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto contiguous_is_empty(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_capacity(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_is_shared(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto object_clone(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& receiver_type,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto object_view(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_access(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver_source,
      LLVMValueRef receiver,
      const Tetrodotoxin::Library::Language::Model::Pack& element_default) const
      -> Bool;
  auto object_reserve(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver_source,
      LLVMValueRef receiver,
      LLVMValueRef count,
      const Tetrodotoxin::Library::Language::Model::Pack& element_default) const
      -> Bool;
  auto borrow_fixed(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto slice_view(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver,
      LLVMValueRef start,
      LLVMValueRef count) const -> Bool;
  auto construct(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& type,
      const Tetrodotoxin::Library::Language::Model::Pack& values) const -> Bool;
  auto construct_provider(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Type& type,
      const Tetrodotoxin::Library::Language::Model::Pack& arguments,
      Perimortem::Core::View::Vector<const Ttx::Model::Addressable*> parameters)
      const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
