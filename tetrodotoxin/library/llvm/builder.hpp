// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Builder exposes the physical operations for one executable lowering
// transaction. Library owners retain semantic traversal and control flow state
// while Builder writes directly into the Body owned by this transaction.
class Builder {
 public:
  enum class Arithmetic : U8 {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
  };

  enum class Comparison : U8 {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
  };

  enum class Logical : U8 {
    And,
    Or,
  };

  enum class LoopAction : U8 {
    Break,
    Continue,
  };

  enum class Write : U8 {
    Assign,
    Add,
    Subtract,
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

  // Branch retains alternate and merge blocks while its semantic owner lowers
  // the body and optional alternate Statement.
  class Branch {
   public:
    constexpr Branch(LLVMBasicBlockRef alternate, LLVMBasicBlockRef done)
        : alternate(alternate), done(done) {}

    constexpr auto get_alternate() const -> LLVMBasicBlockRef {
      return alternate;
    }

    constexpr auto get_done() const -> LLVMBasicBlockRef { return done; }

    constexpr auto has_alternate() const -> Bool { return alternate_started; }

    constexpr auto body_reaches_done() const -> Bool { return body_reaches; }

    constexpr auto begin_alternate(Bool reaches_done) -> void {
      alternate_started = True;
      body_reaches = reaches_done;
    }

   private:
    LLVMBasicBlockRef alternate;
    LLVMBasicBlockRef done;
    Bool alternate_started = False;
    Bool body_reaches = False;
  };

  // Match retains the selected input and final merge block while Match lowers
  // its authored cases in order.
  class Match {
   public:
    constexpr Match(LLVMValueRef input, LLVMBasicBlockRef done)
        : input(input), done(done) {}

    constexpr auto get_input() const -> LLVMValueRef { return input; }

    constexpr auto get_done() const -> LLVMBasicBlockRef { return done; }

    constexpr auto reaches_done() const -> Bool { return reaches; }

    constexpr auto set_reaches_done() -> void { reaches = True; }

   private:
    LLVMValueRef input;
    LLVMBasicBlockRef done;
    Bool reaches = False;
  };

  // MatchCase retains one case cleanup boundary and the next comparison block.
  class MatchCase {
   public:
    constexpr MatchCase(
        Count storage_depth,
        Perimortem::Core::Option<LLVMBasicBlockRef> next)
        : storage_depth(storage_depth), next(next) {}

    constexpr auto get_storage_depth() const -> Count { return storage_depth; }

    constexpr auto get_next() const
        -> Perimortem::Core::Option<LLVMBasicBlockRef> {
      return next;
    }

   private:
    Count storage_depth;
    Perimortem::Core::Option<LLVMBasicBlockRef> next;
  };

  constexpr Builder(Body& body) : body(body) {}
  Builder(const Builder&) = delete;
  Builder(Builder&&) = delete;
  auto operator=(const Builder&) -> Builder& = delete;
  auto operator=(Builder&&) -> Builder& = delete;

  constexpr auto get_program() const -> Program& { return body.get_program(); }

  auto arithmetic(
      Arithmetic operation,
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& left,
      const Ttx::Model::Pack& right) const -> Bool;
  auto negate(
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& operand) const -> Bool;
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
  auto compare(
      Comparison operation,
      const Ttx::Model::Type& carrier,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& left,
      const Ttx::Model::Pack& right) const -> Bool;
  auto compare_bytes(
      Comparison operation,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Pack& left,
      const Ttx::Model::Pack& right) const -> Bool;
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
  auto return_values(const Ttx::Model::Pack& values) const -> Bool;
  auto leave_loop(LoopAction action, const Ttx::Concept::Abstract& target) const
      -> Bool;
  auto begin_branch(const Ttx::Model::Pack& condition) const
      -> Perimortem::Core::Option<Branch>;
  auto begin_alternate(Branch& state) const -> Bool;
  auto end_branch(Branch state) const -> Bool;
  auto begin_while(const Ttx::Concept::Abstract& owner) const -> Bool;
  auto select_while(
      const Ttx::Concept::Abstract& owner,
      const Ttx::Model::Pack& condition) const -> Bool;
  auto end_while(const Ttx::Concept::Abstract& owner) const -> Bool;
  auto begin_sequence(
      const Ttx::Concept::Abstract& owner,
      const Ttx::Model::Addressable& binding,
      const Ttx::Model::Pack& input) const -> Bool;
  auto begin_enumeration(
      const Ttx::Concept::Abstract& owner,
      const Ttx::Concept::Layout& bindings,
      Perimortem::Core::View::Vector<U64> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names) const
      -> Bool;
  auto end_iteration(const Ttx::Concept::Abstract& owner) const -> Bool;
  auto begin_match(const Ttx::Model::Pack& input) const
      -> Perimortem::Core::Option<Match>;
  auto begin_constant_case(Match& state, const Ttx::Model::Pack& constant) const
      -> Perimortem::Core::Option<MatchCase>;
  auto begin_value_case(
      Match& state,
      const Ttx::Model::Addressable& payload,
      Ttx::Lexical::Anchor anchor) const -> Perimortem::Core::Option<MatchCase>;
  auto end_match_case(Match& state, MatchCase selected) const -> Bool;
  auto begin_default_case() const -> MatchCase;
  auto end_match(Match state, Bool unmatched_reaches_next) const -> Bool;
  auto begin_function(
      const Ttx::Model::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) const -> Bool;
  auto parameter(
      const Ttx::Model::Addressable& parameter,
      Ttx::Lexical::Anchor anchor,
      Count index) const -> Bool;
  auto end_function() const -> Bool;
  auto begin_block(
      const Ttx::Concept::Abstract& block,
      Ttx::Lexical::Anchor anchor) const -> Bool;
  auto end_block(const Ttx::Concept::Abstract& block) const -> Bool;
  auto statement(Ttx::Lexical::Anchor anchor) const -> Bool;
  auto local(const Ttx::Model::Addressable& local, Ttx::Lexical::Anchor anchor)
      const -> Bool;
  auto has_full_debug() const -> Bool;
  auto constant_local(
      const Ttx::Model::Addressable& local,
      const Ttx::Model::Pack& value,
      Ttx::Lexical::Anchor anchor) const -> Bool;
  auto end_statement() const -> Bool;
  auto bind_local(
      const Ttx::Model::Addressable& local,
      const Ttx::Model::Pack& value) const -> Bool;
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
  Body& body;
};

}  // namespace Tetrodotoxin::Library::Llvm
