// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Match owns one authored value selection. It evaluates one scalar input and
// compares folded Constant cases in source order. Every selected body is a real
// nested Block, so the first equal case is the complete control destination and
// cannot fall through into another case.
class Match : public Ttx::Concept::Abstract {
 private:
  struct Case {
    Ttx::Concept::Reference<Expression> expression;
    Ttx::Concept::Reference<Block> body;
    Perimortem::Core::Option<Ttx::Concept::Reference<const Constant>> constant;
  };

 public:
  TTX_CONTRACT(
      Match,
      Ttx::Concept::Abstract,
      0x97002fb2d2fb41f8,
      0x99126689dfa4e6bb);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Block& lexical_context,
      Ttx::Model::Callable& function,
      const Ttx::Model::Type& access_scope) -> Perimortem::Core::Option<Match&>;

  Match(const Match&) = delete;
  Match(Match&&) = delete;
  auto operator=(const Match&) -> Match& = delete;
  auto operator=(Match&&) -> Match& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      const Ttx::Model::Type& access_scope) -> Bool;

  auto finalize() -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Match"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_input() const -> const Expression& { return input.get(); }

  constexpr auto get_case_count() const -> Count { return cases.get_size(); }

  auto get_case_constant(Count index) const
      -> Perimortem::Core::Option<const Constant&>;

  auto get_case_body(Count index) const
      -> Perimortem::Core::Option<const Block&>;

  constexpr auto get_default() const -> Perimortem::Core::Option<const Block&> {
    return default_body.visit(
        []() -> Perimortem::Core::Option<const Block&> { return {}; },
        [](const Ttx::Concept::Reference<Block>& selected)
            -> Perimortem::Core::Option<const Block&> {
          return selected.get();
        });
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
