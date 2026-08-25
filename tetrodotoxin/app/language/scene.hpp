// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/app/language/route.hpp"
#include "tetrodotoxin/app/language/transition.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Language {

// Scene owns the application policy for one initial Scene and an ordered set of
// Signal transitions. It does not own the live stack. Linking replaces every
// route with relationships to the real Scene and Signal graph identities.
class Scene : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Scene, Ttx::Concept::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      Route initial,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Transition>>
          transitions,
      Ttx::Lexical::Anchor anchor) -> Scene&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      Route initial,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Transition>>
          transitions) -> Scene&;

  auto link(Ttx::Lexical::Cursor& cursor, const Ttx::Concept::Abstract& context)
      -> Bool;
  auto link_restored(const Ttx::Concept::Abstract& context) -> Bool;

  constexpr auto get_initial_route() const -> const Route& { return initial; }
  constexpr auto get_initial_scene() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Scene::Language::Monograph&> {
    return initial_scene.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Scene::Language::Monograph&> {
          return {};
        },
        [](const Ttx::Concept::Reference<
            const Tetrodotoxin::Scene::Language::Monograph>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Scene::Language::Monograph&> {
          return selected.get();
        });
  }
  constexpr auto get_transitions() const { return transitions; }

  TTX_NAME("Scene"_view);
  TTX_DOCUMENTATION(documentation);

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

 private:
  constexpr Scene(
      const Ttx::Concept::Documentation& documentation,
      Route initial,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Transition>>
          transitions,
      Ttx::Lexical::Anchor anchor)
      : documentation(documentation),
        initial(initial),
        transitions(transitions),
        anchor(anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Route initial;
  Perimortem::Core::View::Vector<Ttx::Concept::Reference<Transition>>
      transitions;
  Ttx::Lexical::Anchor anchor;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Tetrodotoxin::Scene::Language::Monograph>>
      initial_scene;
};

}  // namespace Tetrodotoxin::App::Language
