// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "tetrodotoxin/language/import.hpp"

namespace Tetrodotoxin::Library::Language {

// TypeReference keeps the authored access sequence and the edge it establishes
// when its context becomes ready. Native linking uses the selected Type, while
// boundary queries follow the retained reference and can stop at its Import.
// Resolving a Type therefore does not discard how another source supplies it.
//
// The reference remains a value in its owner's syntax storage. Its bound view
// borrows that value, so the owner finishes moving or growing its storage
// before exposing the view and keeps the observation stable while it is
// consumed.
class TypeReference {
 public:
  // Native linking can use the resolved Type while the retained reference
  // still answers policy questions along its authored access sequence. The
  // first Import encountered owns that dependency. Later accesses remain
  // relative to it, rather than becoming declarations copied from its source.
  auto get_interface() const -> Ttx::Concept::Abstract::Handle;
  auto bind_interface(U64 requested) const
      -> Perimortem::Utility::Result<Ttx::Concept::Binding,
                                     Ttx::Concept::Binding::Failure>;
  using Argument = Perimortem::Core::Static::
      Union<const TypeReference&, const Ttx::Concept::Abstract&>;

  class Failure {
   public:
    enum class Type : U8 {
      Route,
      Argument,
      Generic,
      Unavailable,
      Arity,
      Parameter,
      Recursive,
      Formula,
    };

    constexpr Failure(Type type, Ttx::Lexical::Anchor anchor, Count index = 0)
        : type(type), anchor(anchor), index(index) {}

    constexpr auto get_type() const -> Type { return type; }

    constexpr auto get_index() const -> Count { return index; }

    constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

   private:
    Type type;
    Ttx::Lexical::Anchor anchor;
    Count index;
  };

  using Resolution =
      Perimortem::Utility::Result<const Ttx::Concept::Abstract&, Failure>;

  static constexpr auto create_authored(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Token terminal,
      Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>>
          arguments = {}) -> TypeReference {
    return TypeReference(route, anchor, terminal, arguments);
  }

  static constexpr auto create(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Token terminal,
      Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>>
          arguments = {}) -> TypeReference {
    return TypeReference(route, anchor, terminal, arguments);
  }

  auto get_size() const -> Count;

  auto get_name(Count index) const -> Perimortem::Core::View::Bytes;

  // Authored routes are contiguous, so each segment Token remains recoverable
  // without retaining a second parser representation.
  auto get_token(Count index) const -> Ttx::Lexical::Token;

  constexpr auto get_root() const -> Perimortem::Core::View::Bytes {
    return get_name(0);
  }

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto has_arguments() const -> Bool { return Bool(arguments); }

  constexpr auto get_argument_size() const -> Count {
    return arguments.visit(
        []() -> Count { return 0; },
        [](const auto& selected) -> Count { return selected.get_size(); });
  }

  // Alias completion revisits nested authored routes because their targets may
  // still be settling in the same Type barrier. Literal arguments already name
  // stable identities that the later materialization query can use directly.
  auto get_argument_reference(Count index) const
      -> Perimortem::Core::Option<const TypeReference&>;

  auto get_argument(Count index) const
      -> Perimortem::Core::Option<const Argument&>;

  // Route matching helps declaration owners that still have only authored
  // spelling. Once Generic arguments appear, their selected Generic owns the
  // semantic identity and textual comparison no longer answers the same
  // question.
  auto matches_route(const TypeReference& other) const -> Bool;

  // Public resolution follows the route exactly as another graph consumer sees
  // it. The supplied context resolves the root, then each selected identity
  // answers the next segment.
  auto resolve(const Ttx::Concept::Abstract& context) const -> Resolution;

  // Alias completion starts from the declaration's real host, where the root
  // name has its lexical authority. Each suffix then follows the identity
  // selected by the segment before it.
  auto resolve_lexical(const Ttx::Concept::Abstract& context) const
      -> Resolution;

  // Declaration owners can publish a typed failure as soon as the route
  // settles. Alias closure has no Cursor, but its forward target may still
  // complete during the same Type barrier.
  auto resolve_authored(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  auto report(Ttx::Lexical::Cursor& cursor, const Failure& failure) const
      -> void;

 private:
  enum class Root : U8 {
    Context,
    Lexical,
  };

  auto resolve_with_root(
      const Ttx::Concept::Abstract& context,
      Root root,
      Perimortem::Core::Option<Ttx::Lexical::Cursor&> cursor = {}) const
      -> Resolution;

  constexpr TypeReference(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Token terminal,
      Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>>
          arguments = {})
      : route(route),
        anchor(anchor),
        terminal(terminal),
        arguments(arguments) {}

  mutable Perimortem::Core::Option<Tetrodotoxin::Language::Import::Handle>
      dependency;
  mutable Count dependency_suffix = 0;
  mutable const Ttx::Concept::Abstract* target = nullptr;
  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Anchor anchor;
  Ttx::Lexical::Token terminal;
  Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>> arguments;
};

}  // namespace Tetrodotoxin::Library::Language
