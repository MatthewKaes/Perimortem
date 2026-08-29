// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/model/layout.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Archive {

auto write(Writer& writer, const Language::TypeReference& reference) -> Bool;

auto read_type_reference(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& context)
    -> Perimortem::Core::Option<Language::TypeReference>;

auto write(Writer& writer, const Language::Model::Layout& layout) -> Bool;

auto read_layout(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& context,
    Bool parameters) -> Perimortem::Core::Option<Language::Model::Layout&>;

auto write(Writer& writer, const Language::Signature& signature) -> Bool;

auto read_signature(
    Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& host)
    -> Perimortem::Core::Option<Language::Signature&>;

}  // namespace Tetrodotoxin::Library::Archive
