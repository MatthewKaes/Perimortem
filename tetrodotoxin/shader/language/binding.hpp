// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/render/language/binding.hpp"

namespace Tetrodotoxin::Shader::Language {

// Binding adds Shader storage meaning to one real Library Field. The Field
// remains the Addressable, Type, initializer, and source identity used by
// executable code. Keeping this relationship identity free lets Shader and
// Render compare policy without copying the value graph they describe. A
// material resource may also select its generated runtime Field on Instance.
class Binding {
 public:
  constexpr Binding(
      Tetrodotoxin::Library::Language::Field& field,
      Tetrodotoxin::Render::Language::Binding::Kind kind,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Field&>
          instance_field = {})
      : field(field), kind(kind), instance_field(instance_field) {}

  constexpr auto get_field() const
      -> const Tetrodotoxin::Library::Language::Field& {
    return field;
  }

  constexpr auto edit_field() -> Tetrodotoxin::Library::Language::Field& {
    return field;
  }

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return field.get_definition();
  }

  constexpr auto get_kind() const
      -> Tetrodotoxin::Render::Language::Binding::Kind {
    return kind;
  }

  constexpr auto get_instance_field() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Library::Language::Field&> {
    return instance_field.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Library::Language::Field&> { return {}; },
        [](Tetrodotoxin::Library::Language::Field& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Library::Language::Field&> {
          return selected;
        });
  }

 private:
  Tetrodotoxin::Library::Language::Field& field;
  Tetrodotoxin::Render::Language::Binding::Kind kind;
  Perimortem::Core::Option<Tetrodotoxin::Library::Language::Field&>
      instance_field;
};

}  // namespace Tetrodotoxin::Shader::Language
