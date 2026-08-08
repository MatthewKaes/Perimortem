// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

Types::Object::Object(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Monograph& source,
    Materializations& materializations,
    const Structure& source_scope,
    Anchor anchor,
    Anchor name_anchor)
    : Structure(
          domain,
          name,
          documentation,
          visibility,
          source,
          materializations,
          Ttx::Concept::Reference<const Ttx::Model::Type>(source_scope),
          anchor,
          name_anchor) {}

auto Types::Object::create(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Monograph& source,
    Materializations& materializations,
    const Structure& source_scope,
    Anchor anchor,
    Anchor name_anchor) -> Object& {
  return domain.construct_from<Object>([&]() -> Object {
    return Object(
        domain, name, documentation, visibility, source, materializations,
        source_scope, anchor, name_anchor);
  });
}
