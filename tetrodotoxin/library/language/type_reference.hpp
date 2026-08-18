// Perimortem Engine
// Copyright © Matt Kaes

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

namespace Tetrodotoxin::Library::Language {

// TypeReference is one authored Type route with no semantic identity and an
// optional Generic argument shape. The Anchor keeps exact lexical evidence
// while every route remains one source view. Linking splits that spelling only
// to issue the next contextual query. It does not build a parallel path graph
// or preselect a semantic category for any intermediate segment. The source
// transaction Arena remains alive with its completed graph, so delayed linking
// does not copy the spelling. Nested TypeReferences and literal identities
// remain source facts until the declaration owner links the shape.
class TypeReference {
 public:
  class Failure {
   public:
    enum class Type : Unsigned_8 {
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

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<TypeReference>;

  // Consumes only a qualified Type route. Owners whose grammar excludes
  // Generic arguments use this boundary instead of accepting every form a
  // complete TypeReference may represent.
  static auto parse_route(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<TypeReference>;

  auto get_size() const -> Count;

  auto get_name(Count index) const -> Perimortem::Core::View::Bytes;

  constexpr auto get_root() const -> Perimortem::Core::View::Bytes {
    return get_name(0);
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto has_arguments() const -> Bool { return Bool(arguments); }

  constexpr auto get_argument_size() const -> Count {
    return arguments.visit(
        []() -> Count { return 0; },
        [](const auto& selected) -> Count { return selected.get_size(); });
  }

  // Alias completion recursively advances only nested authored routes. Literal
  // arguments are already stable semantic identities and remain private to the
  // materialization query that cannot mutate the literal.
  auto get_argument_reference(Count index) const
      -> Perimortem::Core::Option<const TypeReference&>;

  // Owners with only a route compare the exact contextual spelling they
  // retained. Generic application is deliberately excluded because the selected
  // Generic, not authored syntax comparison, owns semantic argument identity.
  auto matches_route(const TypeReference& other) const -> Bool;

  // Ordinary resolution asks the supplied graph context for the root and every
  // selected identity for its next segment. It carries no declaration
  // authority, so publication can replay an authored route exactly as an
  // external consumer would observe it.
  auto resolve(const Ttx::Concept::Abstract& context) const -> Resolution;

  // Alias completion has no Cursor but still begins at its actual declaration
  // host. Only that first name receives lexical lookup. Every explicit suffix
  // remains an ordinary query on the identity selected before it.
  auto resolve_lexical(const Ttx::Concept::Abstract& context) const
      -> Resolution;

  // Committed declaration owners publish the typed failure immediately.
  // Alias closure alone uses source free resolution while forward targets may
  // still settle during the same Type barrier.
  auto resolve_authored(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  auto report(Ttx::Lexical::Cursor& cursor, const Failure& failure) const
      -> void;

 private:
  enum class Root : Unsigned_8 {
    Context,
    Lexical,
  };

  using Argument = Perimortem::Core::Static::
      Union<const TypeReference&, const Ttx::Concept::Abstract&>;

  auto resolve_with_root(
      const Ttx::Concept::Abstract& context,
      Root root,
      Perimortem::Core::Option<Ttx::Lexical::Cursor&> cursor = {}) const
      -> Resolution;

  constexpr TypeReference(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor,
      Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>>
          arguments = {})
      : route(route), anchor(anchor), arguments(arguments) {}

  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Anchor anchor;
  Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>> arguments;
};

}  // namespace Tetrodotoxin::Library::Language
