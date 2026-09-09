// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/uuid.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/concept/bound.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Language {

// Import is one authored Alias whose external source or Package root is
// acquired by Environment. Its optional Type route resolves over that root,
// then the Alias represents the exact selected identity without forwarding a
// second lookup or Type surface.
class Import : public Ttx::Model::Alias {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    0x01a084b0c85e7fd8,
    0xaec36a6045311647,
  };

  enum class Kind : U8 {
    Source,
    Package,
  };

  // An Import answers where an edge leaves the current source. Its access
  // sequence is relative to the imported root, so a package assembler can
  // retain that dependency without discovering the dependency's declarations.
  // Each access selects a public Type name using this Import's lookup policy.
  struct Operations {
    auto (*get_kind)(const void*) -> Kind;
    auto (*get_locator)(const void*) -> Perimortem::Core::View::Bytes;
    auto (*get_version)(const void*) -> Perimortem::System::Version;
    auto (*get_access_count)(const void*) -> Count;
    auto (*get_access)(const void*, Count)
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  };

  class Handle : public Ttx::Concept::Bound<Operations> {
   public:
    using Bound::Bound;

    auto get_kind() const -> Kind { return operations.get_kind(source); }

    auto get_locator() const -> Perimortem::Core::View::Bytes {
      return operations.get_locator(source);
    }

    auto get_version() const -> Perimortem::System::Version {
      return operations.get_version(source);
    }

    auto get_access_count() const -> Count {
      return operations.get_access_count(source);
    }

    auto get_access(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return operations.get_access(source, index);
    }
  };

  auto bind_interface(Perimortem::System::Uuid requested) const
      -> Perimortem::Utility::Result<
          Ttx::Concept::Binding,
          Ttx::Concept::Binding::Failure> override;

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
      : Ttx::Model::Alias(
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

  TTX_CONTRACT(Import, Ttx::Model::Alias);

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

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 private:
  auto select_target(Perimortem::Core::Option<Ttx::Lexical::Cursor&> cursor)
      const -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Concept::Documentation& local_documentation;
  Perimortem::Core::Option<const Ttx::Concept::Documentation&>
      visible_documentation;
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
