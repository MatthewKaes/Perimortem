// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/binding.hpp"
#include "tetrodotoxin/language/provider.h"
#include "tetrodotoxin/language/reference.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Import is one authored Alias whose external source or Package root is
// acquired by Environment. Its optional Type route resolves over that root,
// then the Alias represents the exact selected identity without forwarding a
// second lookup or Type surface.
class Import : public Ttx::Abstract {
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

  Import(
      Perimortem::Memory::Allocator::Arena& arena,
      const Description& description);
  auto get_name() const -> Perimortem::Core::View::Bytes override {
    return description.get_name();
  }
  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return description.get_documentation();
  }
  auto get_visibility() const -> Visibility {
    return description.get_visibility();
  }
  auto get_kind() const -> Kind { return description.get_kind(); }
  auto get_locator() const -> Perimortem::Core::View::Bytes {
    return description.get_locator();
  }
  auto get_version() const -> Perimortem::System::Version {
    return description.get_version();
  }
  auto get_route() const -> Perimortem::Core::View::Bytes {
    return description.get_route();
  }
  auto get_declaration_anchor() const -> Ttx::Lexical::Anchor {
    return description.get_declaration_anchor();
  }
  auto get_expression_anchor() const -> Ttx::Lexical::Anchor {
    return description.get_expression_anchor();
  }
  auto acquire(ttx_abstract authority) -> Bool;
  auto get_acquired() const -> ttx_abstract { return acquired; }
  auto validate(Ttx::Lexical::Cursor& cursor) const -> Bool;
  auto resolve(ttx_abstract self) const -> ttx_abstract override;
  auto dependency() -> tetrodotoxin_source_dependency;

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  const Description description;
  const Perimortem::Core::View::Bytes version_text;
  ttx_abstract acquired = ttx_unknown();
  ttx_abstract target = ttx_unknown();
};

}  // namespace Tetrodotoxin::Language
