// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "backend/llvm/representation/body.hpp"
#include "llvm-c/Types.h"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Backend::Llvm::Emission {

// Invocation owns callable ABI, Object operations, borrowed views, and
// aggregate construction for one native Body.
class Invocation {
 public:
  constexpr Invocation(Representation::Body& body) : body(body) {}
  Invocation(const Invocation&) = delete;
  Invocation(Invocation&&) = delete;
  auto operator=(const Invocation&) -> Invocation& = delete;
  auto operator=(Invocation&&) -> Invocation& = delete;

  constexpr auto get_program() const -> Representation::Program& {
    return body.get_program();
  }

  auto invoke(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<const Ttx::Model::Pack&> receiver_source) const
      -> Bool;
  auto fit_input(
      const Ttx::Model::Addressable& parameter,
      const Ttx::Model::Pack& source,
      Count offset,
      Count size) const -> Perimortem::Core::Option<LLVMValueRef>;
  auto get_size(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto contiguous_is_empty(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_capacity(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_is_shared(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Ttx::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto object_clone(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& receiver_type,
      const Ttx::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto object_view(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver) const -> Bool;
  auto object_access(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Ttx::Model::Pack& receiver_source,
      LLVMValueRef receiver,
      const Ttx::Model::Pack& element_default) const -> Bool;
  auto object_reserve(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Ttx::Model::Pack& receiver_source,
      LLVMValueRef receiver,
      LLVMValueRef count,
      const Ttx::Model::Pack& element_default) const -> Bool;
  auto borrow_fixed(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      const Ttx::Model::Pack& receiver_source,
      LLVMValueRef receiver) const -> Bool;
  auto slice_view(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      const Ttx::Model::Type& receiver_type,
      LLVMValueRef receiver,
      LLVMValueRef start,
      LLVMValueRef count) const -> Bool;
  auto construct(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& type,
      const Ttx::Model::Pack& values) const -> Bool;
  auto construct_provider(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& type,
      const Ttx::Model::Pack& arguments,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Ttx::Model::Addressable>> parameters)
      const -> Bool;

 private:
  Representation::Body& body;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Emission
