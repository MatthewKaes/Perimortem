// # Tetrodotoxin
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

// States owns literal values, short circuit flow, sum states, and propagation
// for one native Body.
class States {
 public:
  enum class Logical : U8 {
    And,
    Or,
  };

  // Logic retains the source and merge blocks required after the right input
  // of one short circuit operation lowers.
  class Logic {
   public:
    constexpr Logic(LLVMBasicBlockRef left, LLVMBasicBlockRef merge)
        : left(left), merge(merge) {}

    constexpr auto get_left() const -> LLVMBasicBlockRef { return left; }

    constexpr auto get_merge() const -> LLVMBasicBlockRef { return merge; }

   private:
    LLVMBasicBlockRef left;
    LLVMBasicBlockRef merge;
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

  constexpr States(Representation::Body& body) : body(body) {}
  States(const States&) = delete;
  States(States&&) = delete;
  auto operator=(const States&) -> States& = delete;
  auto operator=(States&&) -> States& = delete;

  constexpr auto get_program() const -> Representation::Program& {
    return body.get_program();
  }

  auto unsigned_value(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      U64 value) const -> Bool;
  auto signed_value(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      S64 value) const -> Bool;
  auto real_value(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      R64 value) const -> Bool;
  auto bytes_value(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      Perimortem::Core::View::Bytes value) const -> Bool;
  auto object_value(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result) const -> Bool;
  auto enumeration_name(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& result_type,
      LLVMValueRef value,
      Perimortem::Core::View::Vector<U64> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names) const
      -> Bool;
  auto logical_not(
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& operand) const -> Bool;
  auto begin_logic(Logical operation, const Ttx::Model::Pack& left) const
      -> Perimortem::Core::Option<Logic>;
  auto end_logic(
      Logic state,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& left,
      const Ttx::Model::Pack& right) const -> Bool;
  auto absent(const Ttx::Model::Type& carrier, const Ttx::Model::Pack& result)
      const -> Bool;
  auto present(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& payload) const -> Bool;
  auto result(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& payload) const -> Bool;
  auto begin_unwrap(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& option) const -> Perimortem::Core::Option<Choice>;
  auto end_unwrap(
      Choice state,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& fallback) const -> Bool;
  auto propagate_option(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Type& element,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& option,
      const Ttx::Model::Pack& escape) const -> Bool;
  auto propagate_flag(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& flag,
      const Ttx::Model::Pack& escape) const -> Bool;
  auto propagate_result(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Type& value,
      const Ttx::Model::Type& error,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& source,
      const Ttx::Model::Pack& escape) const -> Bool;

 private:
  Representation::Body& body;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Emission
