// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/definition.hpp"

namespace Tetrodotoxin::Library::Language {

// Authored extends one primary semantic base with one retained Definition.
// Definition supplies identity-free Authorship without entering the semantic
// inheritance chain.
template <typename base_type>
class Authored : public base_type {
 protected:
  template <typename... argument_types>
  constexpr Authored(
      Tetrodotoxin::Language::Definition& definition,
      argument_types&&... arguments)
      : base_type(static_cast<argument_types&&>(arguments)...),
        definition(definition) {}

  auto complete_definition(
      Ttx::Lexical::Token focus,
      Ttx::Lexical::Token closing) -> Bool {
    return definition.complete(focus, closing);
  }

  // A derived owner may need the enclosing mutable transaction while linking
  // its own retained edge. This exposes the real Definition host without
  // making that host semantic parentage or publishing mutable Definition data.
  constexpr auto get_definition_host() -> Ttx::Concept::Abstract& {
    return definition.get_host();
  }

 public:
  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  TTX_NAME(definition.get_name());

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  constexpr auto get_authorship() const
      -> Perimortem::Core::Option<const Ttx::Concept::Authorship&> override {
    return definition.get_authorship();
  }

 private:
  Tetrodotoxin::Language::Definition& definition;
};

}  // namespace Tetrodotoxin::Library::Language
