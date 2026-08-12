// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// TypeReference is one authored, identity-free Type route with an optional
// Generic argument shape. The Anchor keeps exact lexical evidence while every
// segment retains its exact Token and an Arena-stable spelling. Tokens remain
// the authored diagnostic evidence; spellings exist only because delayed link
// cannot recover text from a Token after the source Cursor has gone away.
// Nested TypeReferences and literal identities remain source facts until the
// declaration owner links the shape.
class TypeReference {
 public:
  static auto parse(
      Tetrodotoxin::Language::Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<TypeReference>;

  // Consumes only a qualified Type route. Owners whose grammar excludes
  // Generic arguments use this boundary instead of accepting every form a
  // complete TypeReference may represent.
  static auto parse_route(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<TypeReference>;

  constexpr auto get_size() const -> Count { return names.get_size(); }

  constexpr auto get_name(Count index) const -> Perimortem::Core::View::Bytes {
    return names.get_data()[index];
  }

  constexpr auto get_token(Count index) const -> Ttx::Lexical::Token {
    return tokens.get_data()[index];
  }

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
  // read-only materialization operation.
  auto get_argument_reference(Count index) const
      -> Perimortem::Core::Option<const TypeReference&>;

  // Route-only owners compare the exact contextual spelling they retained.
  // Generic application is deliberately excluded because Materializations,
  // not authored syntax comparison, owns semantic argument identity.
  auto matches_route(const TypeReference& other) const -> Bool;

  auto resolve_route(const Ttx::Concept::Abstract& context) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_route_from(const Ttx::Concept::Abstract& root) const
      -> const Ttx::Concept::Abstract&;

  // Qualification preserves the original caller scope. Alias resolution may
  // redirect an edge, but it never transfers private authority to the target.
  auto resolve_route_from(
      const Ttx::Concept::Abstract& root,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

  // The selected route terminal and argument presence decide the complete
  // declaration result. A bare route must already be a Type; an applied route
  // must be a Generic whose real argument Layout fits its parameter contract.
  auto resolve_type(
      const Ttx::Concept::Abstract& selected,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

  // Publication resolves nested arguments through the same exported policy
  // as the outer route. Generic application never lets a private argument hide
  // behind a publicly named materialized Type.
  auto resolve_exported_type(
      const Ttx::Concept::Abstract& selected,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

 private:
  using Argument = Perimortem::Core::Static::
      Union<const TypeReference&, const Ttx::Concept::Abstract&>;

  constexpr TypeReference(
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Ttx::Lexical::Anchor anchor,
      Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>>
          arguments = {})
      : tokens(tokens), names(names), anchor(anchor), arguments(arguments) {}

  auto resolve_selected(
      const Ttx::Concept::Abstract& selected,
      const Ttx::Model::Type& caller_scope,
      Bool exported) const -> const Ttx::Concept::Abstract&;

  Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Ttx::Lexical::Anchor anchor;
  Perimortem::Core::Option<Perimortem::Core::View::Vector<Argument>> arguments;
};

}  // namespace Tetrodotoxin::Library::Language
