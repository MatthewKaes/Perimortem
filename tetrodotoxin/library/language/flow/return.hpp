// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Return is one concrete terminal statement. It retains one real Pack while the
// enclosing Block supplies lexical lookup, host access, and the Function result
// Layout required during linking. A bare `return` owns an empty Pack, so empty
// flow and flow with several values share one lifecycle without optional state.
class Return : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Return, Ttx::Concept::Abstract);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Return&>;

  Return(const Return&) = delete;
  Return(Return&&) = delete;
  auto operator=(const Return&) -> Return& = delete;
  auto operator=(Return&&) -> Return& = delete;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope,
      const Ttx::Concept::Layout& results) -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  auto lower(Llvm::Builder& body) const -> Bool;

  TTX_NAME("Return"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }
  constexpr auto get_pack() const -> const Model::Pack& { return pack.get(); }

 private:
  constexpr Return(
      Ttx::Lexical::Anchor anchor,
      Ttx::Concept::Reference<Model::Pack> pack)
      : anchor(anchor), pack(pack) {}

  Ttx::Lexical::Anchor anchor;
  Ttx::Concept::Reference<Model::Pack> pack;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
