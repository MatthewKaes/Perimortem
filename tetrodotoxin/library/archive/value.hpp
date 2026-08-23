// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Archive {

auto write_folded(Writer& writer, const Language::Model::Pack& value) -> Bool;

auto read_folded(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& lexical_context)
    -> Perimortem::Core::Option<Language::Model::Pack&>;

}  // namespace Tetrodotoxin::Library::Archive
