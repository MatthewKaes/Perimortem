// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/admission.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Model::admit(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena,
    Pack& source) -> Option<Pack&> {
  auto admission = target.resolve_concept("admission"_view).select<Admission>();
  return admission ? admission->admit(arena, source) : Option<Pack&>();
}
