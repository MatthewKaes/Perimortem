// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Unit carries only target publication facts for one separately compiled
// Package member. Original semantic identities remain the keys, while Archive
// export symbols provide exact declarations for identities owned elsewhere.
class Unit {
 public:
  class Binding {
   public:
    constexpr Binding(
        const Ttx::Concept::Abstract& semantic,
        Perimortem::Core::View::Bytes symbol)
        : semantic(semantic), symbol(symbol) {}

    constexpr auto get_semantic() const -> const Ttx::Concept::Abstract& {
      return semantic.get();
    }

    constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
      return symbol;
    }

   private:
    Ttx::Concept::Reference<const Ttx::Concept::Abstract> semantic;
    Perimortem::Core::View::Bytes symbol;
  };

  constexpr Unit(
      Perimortem::Core::View::Bytes package = {},
      Perimortem::Core::View::Bytes member = {},
      Perimortem::Core::View::Bytes artifact = {},
      Perimortem::Core::View::Vector<Binding> external = {},
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>> local = {})
      : package(package),
        member(member),
        artifact(artifact),
        external(external),
        local(local) {}

  constexpr auto bind(const Ttx::Concept::Abstract& semantic) const -> Unit {
    return Unit(
        package, member, artifact, external,
        Ttx::Concept::Reference<const Ttx::Concept::Abstract>(semantic));
  }

  constexpr auto owns(const Ttx::Concept::Abstract& semantic) const -> Bool {
    return local && &local->get() == &semantic;
  }

  constexpr auto get_package() const -> Perimortem::Core::View::Bytes {
    return package;
  }

  constexpr auto get_member() const -> Perimortem::Core::View::Bytes {
    return member;
  }

  constexpr auto get_artifact() const -> Perimortem::Core::View::Bytes {
    return artifact;
  }

  constexpr auto is_package_member() const -> Bool {
    return !package.is_empty() && !member.is_empty() && !artifact.is_empty();
  }

  auto find(const Ttx::Concept::Abstract& semantic) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    for (const Binding& binding : external) {
      if (&binding.get_semantic() == &semantic) {
        return binding.get_symbol();
      }
    }
    return {};
  }

 private:
  Perimortem::Core::View::Bytes package;
  Perimortem::Core::View::Bytes member;
  Perimortem::Core::View::Bytes artifact;
  Perimortem::Core::View::Vector<Binding> external;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      local;
};

}  // namespace Tetrodotoxin::Library::Llvm
