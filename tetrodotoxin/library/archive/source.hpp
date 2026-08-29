// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Archive {

auto write(Writer& writer, const Language::Import& import) -> Bool;

auto read_import(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& context)
    -> Perimortem::Core::Option<Language::Import>;

auto write(Writer& writer, const Language::Types::Source& source) -> Bool;

auto read_source(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Language::Types::Source& source) -> Bool;

}  // namespace Tetrodotoxin::Library::Archive
