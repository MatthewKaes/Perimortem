// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Language {

// Import is a contextual Type whose external source or Package root is
// acquired by Environment. Its optional Type route is an ordinary chain of
// contextual queries over that root. The authored local name and Visibility
// place the resulting Type interface in its Monograph without introducing a
// separate dependency table.
class Import : public Ttx::Model::Type {
 public:
  enum class Kind : U8 {
    Source,
    Package,
  };

  class Description {
   public:
    constexpr Description(
        Perimortem::Core::View::Bytes name,
        const Ttx::Concept::Documentation& documentation,
        Visibility visibility,
        Kind kind,
        Perimortem::Core::View::Bytes locator,
        Perimortem::System::Version version,
        Perimortem::Core::View::Bytes route,
        Ttx::Lexical::Anchor declaration_anchor,
        Ttx::Lexical::Anchor expression_anchor,
        Ttx::Lexical::Anchor route_anchor)
        : name(name),
          documentation(documentation),
          visibility(visibility),
          kind(kind),
          locator(locator),
          version(version),
          route(route),
          declaration_anchor(declaration_anchor),
          expression_anchor(expression_anchor),
          route_anchor(route_anchor) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_documentation() const
        -> const Ttx::Concept::Documentation& {
      return documentation;
    }
    constexpr auto get_visibility() const -> Visibility { return visibility; }
    constexpr auto get_kind() const -> Kind { return kind; }
    constexpr auto get_locator() const -> Perimortem::Core::View::Bytes {
      return locator;
    }
    constexpr auto get_version() const -> Perimortem::System::Version {
      return version;
    }
    constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
      return route;
    }
    constexpr auto get_declaration_anchor() const -> Ttx::Lexical::Anchor {
      return declaration_anchor;
    }
    constexpr auto get_expression_anchor() const -> Ttx::Lexical::Anchor {
      return expression_anchor;
    }
    constexpr auto get_route_anchor() const -> Ttx::Lexical::Anchor {
      return route_anchor;
    }

   private:
    Perimortem::Core::View::Bytes name;
    const Ttx::Concept::Documentation& documentation;
    Visibility visibility;
    Kind kind;
    Perimortem::Core::View::Bytes locator;
    Perimortem::System::Version version;
    Perimortem::Core::View::Bytes route;
    Ttx::Lexical::Anchor declaration_anchor;
    Ttx::Lexical::Anchor expression_anchor;
    Ttx::Lexical::Anchor route_anchor;
  };

  constexpr Import(
      Perimortem::Memory::Allocator::Arena& domain,
      const Description& description)
      : domain(domain),
        local_documentation(description.get_documentation()),
        name(description.get_name()),
        visibility(description.get_visibility()),
        kind(description.get_kind()),
        locator(description.get_locator()),
        version(description.get_version()),
        route(description.get_route()),
        declaration_anchor(description.get_declaration_anchor()),
        expression_anchor(description.get_expression_anchor()),
        route_anchor(description.get_route_anchor()) {}

  TTX_CONTRACT(Import, Ttx::Model::Type);
  TTX_NAME(name);

  constexpr auto get_visibility() const -> Visibility { return visibility; }
  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_locator() const -> Perimortem::Core::View::Bytes {
    return locator;
  }
  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }
  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }
  constexpr auto get_declaration_anchor() const -> Ttx::Lexical::Anchor {
    return declaration_anchor;
  }
  constexpr auto get_expression_anchor() const -> Ttx::Lexical::Anchor {
    return expression_anchor;
  }

  auto acquire(const Ttx::Model::Type& root) -> Bool;

  auto get_acquired() const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto validate(Ttx::Lexical::Cursor& cursor) -> Bool;
  auto validate_restored() -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes selected) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes selected) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes selected) const
      -> const Ttx::Concept::Abstract& override;

  auto get_layout() const -> const Ttx::Concept::Layout& override;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 private:
  auto select(Perimortem::Core::Option<Ttx::Lexical::Cursor&> cursor) const
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Concept::Documentation& local_documentation;
  Perimortem::Core::Option<const Ttx::Concept::Documentation&>
      visible_documentation;
  Perimortem::Core::View::Bytes name;
  Visibility visibility;
  Kind kind;
  Perimortem::Core::View::Bytes locator;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Anchor declaration_anchor;
  Ttx::Lexical::Anchor expression_anchor;
  Ttx::Lexical::Anchor route_anchor;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      acquired;
};

}  // namespace Tetrodotoxin::Language
