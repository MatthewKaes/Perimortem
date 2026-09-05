// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "ttx/reference/concept/layout.hpp"
#include "ttx/ffi/cpp/addressable.hpp"
#include "ttx/ffi/cpp/callable.hpp"
#include "ttx/ffi/cpp/domain.hpp"
#include "ttx/lexical/anchor.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

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
        const Ttx::Model::Domain& element,
        LLVMValueRef data,
        LLVMValueRef length,
        LLVMValueRef first)
        : element(&element), data(data), length(length), first(first) {}

    constexpr auto get_element() const -> const Ttx::Model::Domain& {
      return *element;
    }

    constexpr auto get_data() const -> LLVMValueRef { return data; }

    constexpr auto get_length() const -> LLVMValueRef { return length; }

    constexpr auto get_first() const -> LLVMValueRef { return first; }

   private:
    const Ttx::Model::Domain* element;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef first;
  };

  constexpr Storage(Module::Body& body) : body(body) {}
  Storage(const Storage&) = delete;
  Storage(Storage&&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;
  auto operator=(Storage&&) -> Storage& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto range(
      const Ttx::Model::Domain& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& start,
      const Tetrodotoxin::Library::Language::Model::Pack& end) const -> Bool;
  auto empty_range(
      const Ttx::Model::Domain& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool;
  auto select_index(
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver,
      const Tetrodotoxin::Library::Language::Model::Pack& index) const -> Bool;
  auto select_range(
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver,
      const Tetrodotoxin::Library::Language::Model::Pack& start,
      const Tetrodotoxin::Library::Language::Model::Pack& count,
      Count size) const -> Bool;
  auto begin_slice(
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver,
      const Tetrodotoxin::Library::Language::Model::Pack& index) const
      -> Perimortem::Core::Option<Choice>;
  auto end_slice(
      Choice state,
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
      -> Bool;
  auto begin_slice_range(
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver,
      const Tetrodotoxin::Library::Language::Model::Pack& start) const
      -> Perimortem::Core::Option<SliceRange>;
  auto begin_slice_slot(const SliceRange& range, Count offset) const
      -> Perimortem::Core::Option<Choice>;
  auto end_slice_slot(
      Choice state,
      const Ttx::Model::Domain& element,
      const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
      -> Perimortem::Core::Option<LLVMValueRef>;
  auto end_slice_range(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const -> Bool;
  auto select(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Addressable& addressable) const -> Bool;
  auto select_member(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Ttx::Model::Addressable& addressable,
      const Tetrodotoxin::Library::Language::Model::Pack& receiver) const
      -> Bool;
  auto load(const Tetrodotoxin::Library::Language::Model::Pack& result) const
      -> Bool;
  auto compose(const Tetrodotoxin::Library::Language::Model::Pack& result) const
      -> Bool;
  auto alias(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& source) const -> Bool;
  auto write(
      Write kind,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& target,
      const Tetrodotoxin::Library::Language::Model::Pack& source) const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
