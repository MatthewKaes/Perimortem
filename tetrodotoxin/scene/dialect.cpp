// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/dialect.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/scene/archive/reader.hpp"
#include "tetrodotoxin/scene/archive/writer.hpp"
#include "tetrodotoxin/scene/interpreter/source.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Scene::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Allocator::Arena& arena = cursor.get_arena();
  auto& child = Library::Language::Monograph::create_authored(
      arena, documentation, source_anchor, library, context);
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      arena, documentation, child.get_source(), "Scene"_view,
      Tetrodotoxin::Language::Visibility::Public, source_anchor);
  auto& instance =
      Library::Language::Types::Object::create_synthetic(arena, definition);
  BAIL_IF(!child.get_source().retain_definition(
      instance, Library::Language::Types::Composite::Category::Type, True));

  auto& monograph = Scene::Language::Monograph::create(
      arena, documentation, *this, context, child, instance);
  Scene::Interpreter::Source::parse(monograph, cursor);
  return monograph;
}

auto Scene::Dialect::encode(
    const Abstract& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Dynamic::Bytes> {
  auto scene = monograph.select<Scene::Language::Monograph>();
  BAIL_IF(!scene);
  return Scene::Archive::Writer::encode(*scene, profile);
}

auto Scene::Dialect::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Documentation&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto restored = Scene::Archive::Reader::restore(
      arena, payload, profile, *this, library, context);
  return restored ? Option<Tetrodotoxin::Language::Monograph&>(*restored)
                  : Option<Tetrodotoxin::Language::Monograph&>();
}
