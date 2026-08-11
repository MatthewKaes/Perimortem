// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Defined is the required Definition-bearing Library Type contract. It keeps
// definition provenance on the same public Type inheritance chain without
// turning hosting into semantic parentage or another graph identity.
class Defined : public Ttx::Model::Type {
 protected:
  constexpr Defined(Tetrodotoxin::Language::Definition& definition)
      : definition(definition) {}

 public:
  TTX_CONTRACT(
      Defined,
      Ttx::Model::Type,
      0x9d65310686cf4e6a,
      0xa49952f036ba02c6);

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_host() -> Ttx::Concept::Abstract& {
    return definition.get_host();
  }

  constexpr auto get_host() const -> const Ttx::Concept::Abstract& {
    return definition.get_host();
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  constexpr auto get_authorship() const
      -> Perimortem::Core::Option<const Ttx::Concept::Authorship&> override {
    return definition.get_authorship();
  }

  TTX_NAME(definition.get_name());

  TTX_DOCUMENTATION(definition.get_documentation());

 private:
  Tetrodotoxin::Language::Definition& definition;
};

}  // namespace Tetrodotoxin::Library::Language::Types
