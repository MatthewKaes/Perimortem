// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Return is one concrete terminal statement. It retains one real Pack while the
// enclosing Block supplies lexical lookup, host access, and the Function result
// Layout required during linking. `return;` owns an empty Pack, so empty and
// multi-value flow follow the same lifecycle without optional flow state.
class Return : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(
      Return,
      Ttx::Concept::Abstract,
      0xf235213054c548a0,
      0xae72ff5b04315181);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Return&>;

  Return(const Return&) = delete;
  Return(Return&&) = delete;
  auto operator=(const Return&) -> Return& = delete;
  auto operator=(Return&&) -> Return& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      const Ttx::Model::Type& access_scope,
      const Ttx::Concept::Layout& results) -> Bool;

  auto finalize() -> void;

  TTX_NAME("Return"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr Return(
      Ttx::Lexical::Anchor anchor,
      Ttx::Concept::Reference<Model::Pack> pack)
      : anchor(anchor), pack(pack) {}

  Ttx::Lexical::Anchor anchor;
  Ttx::Concept::Reference<Model::Pack> pack;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
