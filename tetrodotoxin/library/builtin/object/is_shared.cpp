// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/object/is_shared.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Object::IsShared::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> IsShared& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<IsShared>(
      [&]() -> IsShared { return IsShared(self, result); });
}
