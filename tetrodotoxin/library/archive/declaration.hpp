// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

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
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
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

auto write(Writer& writer, const Language::Alias& alias) -> Bool;

auto read_alias(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Language::Alias&>;

auto write(Writer& writer, const Language::Field& field) -> Bool;

auto read_field(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Language::Field&>;

auto write(Writer& writer, const Language::Function& function) -> Bool;

auto read_function(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Function&>;

}  // namespace Tetrodotoxin::Library::Archive
