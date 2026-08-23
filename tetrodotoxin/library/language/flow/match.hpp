// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Match owns one authored value selection. Constant cases compare one folded
// scalar domain. An Option uses one branch local value case for presence and
// the final discard case for absence. A selected body never falls through.
class Match : public Ttx::Concept::Abstract {
 public:
  enum class CaseKind : U8 {
    Constant,
    Value,
  };

 private:
  struct Case {
    CaseKind kind;
    Perimortem::Core::Option<Ttx::Concept::Reference<Expression>> expression;
    Ttx::Concept::Reference<Block> body;
    Perimortem::Core::Option<Ttx::Concept::Reference<Model::Addressable>>
        payload;
    Ttx::Lexical::Anchor anchor;
    Perimortem::Core::Option<Ttx::Concept::Reference<const Constant>> constant;
  };

 public:
  TTX_CONTRACT(Match, Ttx::Concept::Abstract);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Block& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope) -> Perimortem::Core::Option<Match&>;

  Match(const Match&) = delete;
  Match(Match&&) = delete;
  auto operator=(const Match&) -> Match& = delete;
  auto operator=(Match&&) -> Match& = delete;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Match"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_input() const -> const Expression& { return input.get(); }

  constexpr auto get_case_count() const -> Count { return cases.get_size(); }

  auto get_case_constant(Count index) const
      -> Perimortem::Core::Option<const Constant&>;

  auto get_case_kind(Count index) const -> Perimortem::Core::Option<CaseKind>;

  auto get_case_payload(Count index) const
      -> Perimortem::Core::Option<const Model::Addressable&>;

  auto get_case_body(Count index) const
      -> Perimortem::Core::Option<const Block&>;

  auto get_case_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;

  constexpr auto get_default() const -> Perimortem::Core::Option<const Block&> {
    return default_body.visit(
        []() -> Perimortem::Core::Option<const Block&> { return {}; },
        [](const Ttx::Concept::Reference<Block>& selected)
            -> Perimortem::Core::Option<const Block&> {
          return selected.get();
        });
  }

  constexpr auto has_complete_coverage() const -> Bool {
    return complete_coverage;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  Match(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& input,
      Ttx::Lexical::Anchor anchor)
      : input(input), cases(domain), anchor(anchor) {}

  Ttx::Concept::Reference<Expression> input;
  Perimortem::Memory::Managed::Vector<Case> cases;
  Perimortem::Core::Option<Ttx::Concept::Reference<Block>> default_body;
  Ttx::Lexical::Anchor anchor;
  Bool complete_coverage = False;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
