// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"

namespace Tetrodotoxin::Library::Archive {

auto write(Writer& writer, const Language::Foreign::State& state) -> Bool;

auto read_foreign_state(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Language::Foreign& host)
    -> Perimortem::Core::Option<Language::Foreign::State&>;

auto write(Writer& writer, const Language::Foreign::Function& function) -> Bool;

auto read_foreign_function(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Language::Foreign& host)
    -> Perimortem::Core::Option<Language::Foreign::Function&>;

auto write(Writer& writer, const Language::Foreign& foreign) -> Bool;

auto read_foreign(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    Language::Foreign& foreign) -> Bool;

}  // namespace Tetrodotoxin::Library::Archive
