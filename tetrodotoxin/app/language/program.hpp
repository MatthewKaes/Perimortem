// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::App::Language {

// Program retains one authored Package route and the exact Static Callable
// selected from that completed Package graph during linking.
class Program : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Program, Ttx::Concept::Abstract);

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Core::Option<Program&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes callable_name) -> Program&;

  auto link(Ttx::Lexical::Cursor& cursor, Ttx::Concept::Abstract& context)
      -> Bool;
  auto link_restored(Ttx::Concept::Abstract& context) -> Bool;

  TTX_NAME("Program"_view);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_callable_name() const -> Perimortem::Core::View::Bytes {
    return callable_name;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_entry() const
      -> Perimortem::Core::Option<const Ttx::Model::Callable&> {
    return entry ? Perimortem::Core::Option<const Ttx::Model::Callable&>(
                       entry->get())
                 : Perimortem::Core::Option<const Ttx::Model::Callable&>();
  }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

 private:
  constexpr Program(
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes callable_name,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor selection_anchor)
      : documentation(documentation),
        route(route),
        callable_name(callable_name),
        anchor(anchor),
        selection_anchor(selection_anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Bytes route;
  Perimortem::Core::View::Bytes callable_name;
  Ttx::Lexical::Anchor anchor;
  Ttx::Lexical::Anchor selection_anchor;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Callable>>
      entry;
};

}  // namespace Tetrodotoxin::App::Language
