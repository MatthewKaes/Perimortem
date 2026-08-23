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

// Storage owns ranges, indexed selection, address publication, composition, and
// writes for one native Body.
class Storage {
 public:
  enum class Write : U8 {
    Assign,
    Add,
    Subtract,
  };

  // Choice retains one selected value and the two blocks needed to merge a
  // separately lowered fallback.
  class Choice {
   public:
    constexpr Choice(
        LLVMValueRef selected,
        LLVMBasicBlockRef selected_end,
        LLVMBasicBlockRef merge)
        : selected(selected), selected_end(selected_end), merge(merge) {}

    constexpr auto get_selected() const -> LLVMValueRef { return selected; }

    constexpr auto get_selected_end() const -> LLVMBasicBlockRef {
      return selected_end;
    }

    constexpr auto get_merge() const -> LLVMBasicBlockRef { return merge; }

   private:
    LLVMValueRef selected;
    LLVMBasicBlockRef selected_end;
    LLVMBasicBlockRef merge;
  };

  // SliceRange retains one contiguous source while Slice creates an independent
  // fallback and merge for each requested output slot.
  class SliceRange {
   public:
    constexpr SliceRange(
        const Ttx::Model::Type& element,
        LLVMValueRef data,
        LLVMValueRef length,
        LLVMValueRef first)
        : element(element), data(data), length(length), first(first) {}

    constexpr auto get_element() const -> const Ttx::Model::Type& {
      return element.get();
    }

    constexpr auto get_data() const -> LLVMValueRef { return data; }

    constexpr auto get_length() const -> LLVMValueRef { return length; }

    constexpr auto get_first() const -> LLVMValueRef { return first; }

   private:
    Ttx::Concept::Reference<const Ttx::Model::Type> element;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef first;
  };

  constexpr Storage(Representation::Body& body) : body(body) {}
  Storage(const Storage&) = delete;
  Storage(Storage&&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;
  auto operator=(Storage&&) -> Storage& = delete;

  constexpr auto get_program() const -> Representation::Program& {
    return body.get_program();
  }

  auto range(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& start,
      const Ttx::Model::Pack& end) const -> Bool;
  auto empty_range(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result) const -> Bool;
  auto select_index(
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& receiver,
      const Ttx::Model::Pack& index) const -> Bool;
  auto select_range(
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& receiver,
      const Ttx::Model::Pack& start,
      const Ttx::Model::Pack& count,
      Count size) const -> Bool;
  auto begin_slice(
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& receiver,
      const Ttx::Model::Pack& index) const -> Perimortem::Core::Option<Choice>;
  auto end_slice(
      Choice state,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& fallback) const -> Bool;
  auto begin_slice_range(
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& receiver,
      const Ttx::Model::Pack& start) const
      -> Perimortem::Core::Option<SliceRange>;
  auto begin_slice_slot(const SliceRange& range, Count offset) const
      -> Perimortem::Core::Option<Choice>;
  auto end_slice_slot(
      Choice state,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& fallback) const
      -> Perimortem::Core::Option<LLVMValueRef>;
  auto end_slice_range(
      const Ttx::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const -> Bool;
  auto select(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Addressable& addressable) const -> Bool;
  auto select_member(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Addressable& addressable,
      const Ttx::Model::Pack& receiver) const -> Bool;
  auto load(const Ttx::Model::Pack& result) const -> Bool;
  auto compose(const Ttx::Model::Pack& result) const -> Bool;
  auto alias(const Ttx::Model::Pack& result, const Ttx::Model::Pack& source)
      const -> Bool;
  auto write(
      Write kind,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& target,
      const Ttx::Model::Pack& source) const -> Bool;

 private:
  Representation::Body& body;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Emission
