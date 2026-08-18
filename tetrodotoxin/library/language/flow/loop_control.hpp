// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// LoopControl owns one authored `break` or `continue` statement. It retains the
// nearest enclosing loop identity so nested Blocks never reduce that semantic
// relationship to parser depth or a later lowering decision.
class LoopControl : public Ttx::Concept::Abstract {
 public:
  enum class Kind : Unsigned_8 {
    Break,
    Continue,
  };

  TTX_CONTRACT(LoopControl, Ttx::Concept::Abstract);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Block& lexical_context) -> Perimortem::Core::Option<LoopControl&>;

  LoopControl(const LoopControl&) = delete;
  LoopControl(LoopControl&&) = delete;
  auto operator=(const LoopControl&) -> LoopControl& = delete;
  auto operator=(LoopControl&&) -> LoopControl& = delete;

  auto lower(Llvm::Builder& body) const -> Bool;

  TTX_NAME("LoopControl"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_target() const -> const Ttx::Concept::Abstract& {
    return target.get();
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr LoopControl(
      Kind kind,
      const Ttx::Concept::Abstract& target,
      Ttx::Lexical::Anchor anchor)
      : kind(kind), target(target), anchor(anchor) {}

  Kind kind;
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> target;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
