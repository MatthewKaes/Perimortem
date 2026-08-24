// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Shader::Language {

// Bridge names one intentional relationship between two distinct Library Type
// identities used on opposite sides of a graphics boundary. Similar Layouts
// alone cannot establish this relationship because they omit conversion,
// direction, and lifetime policy.
//
// Marshaling explains how values correspond while synchronization explains
// when the receiving side may observe them. These remain Shader meaning because
// CPU and SPIR V Terminals can realize the same relationship differently
// without changing either Type.
class Bridge : public Ttx::Concept::Abstract {
 public:
  enum class Direction : U8 {
    Upload,
    Download,
    Bidirectional,
  };

  enum class Marshaling : U8 {
    Identity,
    Copy,
    Pack,
  };

  enum class Synchronization : U8 {
    None,
    Submission,
    Frame,
  };

  TTX_CONTRACT(Bridge, Ttx::Concept::Abstract);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Library::Language::TypeReference cpu,
      Library::Language::TypeReference gpu,
      Direction direction,
      Marshaling marshaling,
      Synchronization synchronization) -> Bridge&;

  auto link(Ttx::Lexical::Cursor& cursor, const Ttx::Concept::Abstract& context)
      -> Bool;

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());
  TTX_INVALID_CONTEXT;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_direction() const -> Direction { return direction; }
  constexpr auto get_marshaling() const -> Marshaling { return marshaling; }
  constexpr auto get_synchronization() const -> Synchronization {
    return synchronization;
  }

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_cpu_type() const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> {
    return cpu_type.visit(
        []() -> Perimortem::Core::Option<const Ttx::Model::Type&> {
          return {};
        },
        [](const Ttx::Concept::Reference<const Ttx::Model::Type>& selected)
            -> Perimortem::Core::Option<const Ttx::Model::Type&> {
          return selected.get();
        });
  }

  constexpr auto get_gpu_type() const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> {
    return gpu_type.visit(
        []() -> Perimortem::Core::Option<const Ttx::Model::Type&> {
          return {};
        },
        [](const Ttx::Concept::Reference<const Ttx::Model::Type>& selected)
            -> Perimortem::Core::Option<const Ttx::Model::Type&> {
          return selected.get();
        });
  }

 private:
  Bridge(
      Tetrodotoxin::Language::Definition& definition,
      Library::Language::TypeReference cpu,
      Library::Language::TypeReference gpu,
      Direction direction,
      Marshaling marshaling,
      Synchronization synchronization)
      : definition(definition),
        cpu(cpu),
        gpu(gpu),
        direction(direction),
        marshaling(marshaling),
        synchronization(synchronization) {}

  Tetrodotoxin::Language::Definition& definition;
  Library::Language::TypeReference cpu;
  Library::Language::TypeReference gpu;
  Direction direction;
  Marshaling marshaling;
  Synchronization synchronization;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      cpu_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      gpu_type;
};

}  // namespace Tetrodotoxin::Shader::Language
