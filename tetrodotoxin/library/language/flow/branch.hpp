// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Branch owns one complete authored `if` or `while` statement. Its condition
// keeps the complete Pack even though control flow selects the first value.
// Each body is a real nested Block with the enclosing Block as lexical parent.
class Branch : public Ttx::Concept::Abstract {
 public:
  enum class Kind : Unsigned_8 {
    If,
    While,
  };

  TTX_CONTRACT(Branch, Ttx::Concept::Abstract);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Block& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope) -> Perimortem::Core::Option<Branch&>;

  Branch(const Branch&) = delete;
  Branch(Branch&&) = delete;
  auto operator=(const Branch&) -> Branch& = delete;
  auto operator=(Branch&&) -> Branch& = delete;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      Scope& lexical_context,
      const Model::Type& access_scope) -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  auto lower(Llvm::Builder& body) const -> Bool;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Branch"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_condition() const -> const Model::Pack& {
    return condition.get();
  }

  constexpr auto get_body() const -> const Block& { return body->get(); }

  constexpr auto get_alternate() const
      -> Perimortem::Core::Option<const Statement&> {
    return alternate.visit(
        []() -> Perimortem::Core::Option<const Statement&> { return {}; },
        [](const Statement& selected)
            -> Perimortem::Core::Option<const Statement&> { return selected; });
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr Branch(
      Kind kind,
      Model::Pack& condition,
      Ttx::Lexical::Anchor anchor)
      : kind(kind), condition(condition), anchor(anchor) {}

  Kind kind;
  Ttx::Concept::Reference<Model::Pack> condition;
  Perimortem::Core::Option<Ttx::Concept::Reference<Block>> body;
  Perimortem::Core::Option<Statement> alternate;
  Ttx::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
