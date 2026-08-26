// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Language {

// Import is one source-local Alias whose target is acquired by Environment.
// The authored declaration retains only a locator. Once the selected source or
// Package product is present, the Alias binds its real semantic root exactly
// once and every language continues through the ordinary TTX graph.
class Import : public Ttx::Model::Alias {
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
        Ttx::Lexical::Anchor anchor)
        : name(name),
          documentation(documentation),
          visibility(visibility),
          kind(kind),
          locator(locator),
          version(version),
          anchor(anchor) {}

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
    constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

   private:
    Perimortem::Core::View::Bytes name;
    const Ttx::Concept::Documentation& documentation;
    Visibility visibility;
    Kind kind;
    Perimortem::Core::View::Bytes locator;
    Perimortem::System::Version version;
    Ttx::Lexical::Anchor anchor;
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
        anchor(description.get_anchor()) {}

  TTX_CONTRACT(Import, Ttx::Model::Alias);

  constexpr auto get_visibility() const -> Visibility { return visibility; }
  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_locator() const -> Perimortem::Core::View::Bytes {
    return locator;
  }
  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }
  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto bind(const Ttx::Concept::Abstract& target) -> Bool;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Concept::Documentation& local_documentation;
  Perimortem::Core::Option<const Ttx::Concept::Documentation&>
      visible_documentation;
  Visibility visibility;
  Kind kind;
  Perimortem::Core::View::Bytes locator;
  Perimortem::System::Version version;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Language
