// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/binding.hpp"
#include "tetrodotoxin/language/reference.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/ffi/cpp/domain.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Import is one authored Alias whose external source or Package root is
// acquired by Environment. Its optional Type route resolves over that root,
// then the Alias represents the exact selected identity without forwarding a
// second lookup or Type surface.
class Import : public Tetrodotoxin::Language::Binding {
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
      : Tetrodotoxin::Language::Binding(
            description.get_name(),
            description.get_documentation()),
        domain(domain),
        local_documentation(description.get_documentation()),
        visibility(description.get_visibility()),
        kind(description.get_kind()),
        locator(description.get_locator()),
        version(description.get_version()),
        route(description.get_route()),
        declaration_anchor(description.get_declaration_anchor()),
        expression_anchor(description.get_expression_anchor()),
        route_anchor(description.get_route_anchor()) {}


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

  auto acquire(const Ttx::Model::Domain& root) -> Bool;

  auto get_acquired() const
      -> Perimortem::Core::Option<const Ttx::Model::Domain&>;

  auto validate(Ttx::Lexical::Cursor& cursor) -> Bool;
  auto validate_restored() -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 private:
  auto select_target() const -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Concept::Documentation& local_documentation;
  Visibility visibility;
  Kind kind;
  Perimortem::Core::View::Bytes locator;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Anchor declaration_anchor;
  Ttx::Lexical::Anchor expression_anchor;
  Ttx::Lexical::Anchor route_anchor;
  const Ttx::Model::Domain* acquired = nullptr;
  const Ttx::Concept::Abstract* target = nullptr;
};

}  // namespace Tetrodotoxin::Language
