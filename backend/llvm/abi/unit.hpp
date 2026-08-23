// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Backend::Llvm::Abi {

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

  // TypeBinding gives one exact semantic Type its durable Package route. The
  // C header consumes this target fact so every compilation names an imported
  // carrier through its provider rather than the current consumer.
  class TypeBinding {
   public:
    constexpr TypeBinding(
        const Ttx::Model::Type& semantic,
        Perimortem::Core::View::Bytes package,
        Perimortem::Core::View::Bytes member,
        Perimortem::Core::View::Bytes route)
        : semantic(semantic), package(package), member(member), route(route) {}

    constexpr auto get_semantic() const -> const Ttx::Model::Type& {
      return semantic.get();
    }

    constexpr auto get_package() const -> Perimortem::Core::View::Bytes {
      return package;
    }

    constexpr auto get_member() const -> Perimortem::Core::View::Bytes {
      return member;
    }

    constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
      return route;
    }

   private:
    Ttx::Concept::Reference<const Ttx::Model::Type> semantic;
    Perimortem::Core::View::Bytes package;
    Perimortem::Core::View::Bytes member;
    Perimortem::Core::View::Bytes route;
  };

  constexpr Unit(
      Perimortem::Core::View::Bytes package = {},
      Perimortem::Core::View::Bytes member = {},
      Perimortem::Core::View::Bytes artifact = {},
      Perimortem::Core::View::Vector<Binding> external = {},
      Perimortem::Core::View::Vector<TypeBinding> types = {},
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> headers =
          {},
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>> local = {})
      : package(package),
        member(member),
        artifact(artifact),
        external(external),
        types(types),
        headers(headers),
        local(local) {}

  constexpr auto bind(const Ttx::Concept::Abstract& semantic) const -> Unit {
    return Unit(
        package, member, artifact, external, types, headers,
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

  auto find_type(const Ttx::Model::Type& semantic) const
      -> Perimortem::Core::Option<const TypeBinding&> {
    for (Count index = 0; index < types.get_size(); index++) {
      const TypeBinding& binding = types.get_data()[index];
      if (&binding.get_semantic() == &semantic) {
        return binding;
      }
    }
    return {};
  }

  constexpr auto get_headers() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return headers;
  }

 private:
  Perimortem::Core::View::Bytes package;
  Perimortem::Core::View::Bytes member;
  Perimortem::Core::View::Bytes artifact;
  Perimortem::Core::View::Vector<Binding> external;
  Perimortem::Core::View::Vector<TypeBinding> types;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> headers;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      local;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Abi
