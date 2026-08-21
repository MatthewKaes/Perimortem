// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Library::Archive {

// Declaration is the common durable prefix of one Library declaration. It is
// a bounded read value, not a semantic identity. The concrete owner immediately
// uses it to construct its real Definition in the reconstruction Arena.
class Declaration {
 public:
  static auto read(Reader& reader, Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<Declaration>;

  constexpr Declaration(
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility)
      : documentation(documentation),
        attributes(attributes),
        name(name),
        visibility(visibility) {}

  constexpr Declaration(const Tetrodotoxin::Language::Definition& definition)
      : Declaration(
            definition.get_documentation(),
            definition.get_attributes(),
            definition.get_name(),
            definition.get_visibility()) {}

  auto write(Writer& writer) const -> Bool;

  auto create_definition(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host) const
      -> Tetrodotoxin::Language::Definition&;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_visibility() const -> Tetrodotoxin::Language::Visibility {
    return visibility;
  }

 private:
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes;
  Perimortem::Core::View::Bytes name;
  Tetrodotoxin::Language::Visibility visibility;
};

}  // namespace Tetrodotoxin::Library::Archive
