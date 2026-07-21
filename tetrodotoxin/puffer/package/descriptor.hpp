// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/model/dialect.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Tetrodotoxin::Puffer::Package {

// Descriptor is Puffer's parsed package.ttx container transaction. It owns the
// exact external resolutions and explicit member list that must be completed
// before the root Package Source is evaluated. These facts never become Source
// imports or a second semantic package graph.
class Descriptor {
 public:
  class Resolution {
   public:
    constexpr Resolution(
        Perimortem::Core::View::Bytes local_name,
        Perimortem::Core::View::Bytes package_name,
        Perimortem::System::Version version)
        : local_name(local_name),
          package_name(package_name),
          version(version) {}

    constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
      return local_name;
    }

    constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
      return package_name;
    }

    constexpr auto get_version() const -> Perimortem::System::Version {
      return version;
    }

   private:
    Perimortem::Core::View::Bytes local_name;
    Perimortem::Core::View::Bytes package_name;
    Perimortem::System::Version version;
  };

  class Member {
   public:
    constexpr Member(
        Perimortem::Core::View::Bytes local_name,
        const Model::Dialect& dialect,
        Perimortem::Core::View::Bytes path)
        : local_name(local_name), dialect(dialect), path(path) {}

    constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
      return local_name;
    }

    constexpr auto get_dialect() const -> const Model::Dialect& {
      return dialect.get();
    }

    constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
      return path;
    }

   private:
    Perimortem::Core::View::Bytes local_name;
    Ttx::Concept::Reference<Model::Dialect> dialect;
    Perimortem::Core::View::Bytes path;
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Lexical::Tokenizer& tokenizer,
      const Ttx::Concept::Abstract& dialects,
      Ttx::Lexical::Errors& errors) -> const Descriptor*;

  auto evaluate(Model::Source& source, Ttx::Lexical::Errors& errors) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_dialect() const -> const Model::Dialect& {
    return dialect.get();
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto get_resolutions() const
      -> Perimortem::Core::View::Vector<Resolution> {
    return resolutions;
  }

  constexpr auto get_members() const -> Perimortem::Core::View::Vector<Member> {
    return members;
  }

  constexpr auto get_body_token_index() const -> Count {
    return body_token_index;
  }

 private:
  Descriptor(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Dialect& dialect,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path)
      : dialect(dialect),
        documentation(documentation),
        source_text(source_text),
        source_path(source_path),
        resolutions(arena),
        members(arena) {}

  Ttx::Concept::Reference<Model::Dialect> dialect;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Memory::Managed::Vector<Resolution> resolutions;
  Perimortem::Memory::Managed::Vector<Member> members;
  Count body_token_index = 0;
};

}  // namespace Tetrodotoxin::Puffer::Package
