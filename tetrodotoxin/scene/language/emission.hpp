// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Language {

// Emission is the Scene meaning retained by one Library Statement. Library
// still owns source order, lexical scope, and nested control flow, while this
// identity connects an optional value Pack to one exact Signal. The runtime
// Terminal later turns that relationship into an event without placing a live
// queue in either language graph.
class Emission : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Emission, Ttx::Concept::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Concept::Abstract& scene,
      Perimortem::Core::View::Bytes signal_name,
      Ttx::Lexical::Token signal_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
          payload,
      Ttx::Lexical::Anchor anchor) -> Emission&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Library::Language::Flow::Scope& scope) -> Bool;

  auto validate(Ttx::Lexical::Cursor& cursor) const -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  constexpr auto get_signal() const -> Perimortem::Core::Option<const Signal&> {
    return signal;
  }

  constexpr auto get_payload() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Library::Language::Model::Pack&> {
    return payload.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Library::Language::Model::Pack&> {
          return {};
        },
        [](const Tetrodotoxin::Library::Language::Model::Pack& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Library::Language::Model::Pack&> {
          return selected;
        });
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  TTX_NAME(signal_name);
  TTX_EMPTY_DOCUMENTATION();

 private:
  constexpr Emission(
      Ttx::Concept::Abstract& scene,
      Perimortem::Core::View::Bytes signal_name,
      Ttx::Lexical::Token signal_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
          payload,
      Ttx::Lexical::Anchor anchor)
      : scene(scene),
        signal_name(signal_name),
        signal_token(signal_token),
        payload(payload),
        anchor(anchor) {}

  Ttx::Concept::Abstract& scene;
  Perimortem::Core::View::Bytes signal_name;
  Ttx::Lexical::Token signal_token;
  Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
      payload;
  Perimortem::Core::Option<const Signal&> signal;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Scene::Language
