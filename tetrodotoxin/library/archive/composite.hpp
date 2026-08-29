// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/implemented.hpp"
#include "tetrodotoxin/library/language/types/interface.hpp"
#include "tetrodotoxin/library/language/types/namespace.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Archive {

auto write_declarations(
    Writer& writer,
    const Language::Types::Composite& composite) -> Bool;

auto read_declarations(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Language::Types::Composite& composite) -> Bool;

auto write(Writer& writer, const Language::Types::Structure& structure) -> Bool;

auto read_structure(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Structure&>;

auto write(Writer& writer, const Language::Types::Interface& interface) -> Bool;

auto read_interface(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Interface&>;

auto write(Writer& writer, const Language::Types::Namespace& selected) -> Bool;

auto read_namespace(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Namespace&>;

auto write(Writer& writer, const Language::Types::Object& object) -> Bool;

auto read_object(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Object&>;

auto write(Writer& writer, const Language::Types::Implemented& implemented)
    -> Bool;

auto read_implemented(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Implemented&>;

auto write(Writer& writer, const Language::Types::Enumeration& enumeration)
    -> Bool;

auto read_enumeration(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Types::Enumeration&>;

}  // namespace Tetrodotoxin::Library::Archive
