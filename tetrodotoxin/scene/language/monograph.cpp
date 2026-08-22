// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Scene::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Documentation& documentation,
    const Abstract& language,
    Abstract& context,
    Library::Language::Monograph& library)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      library(library) {}

auto Scene::Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  auto outer = Tetrodotoxin::Language::Monograph::get_layer(requested);
  if (outer) {
    return *outer;
  }

  if (&requested == &get_library().get_language()) {
    return get_library();
  }

  return {};
}

auto Scene::Language::Monograph::link(Cursor& cursor) -> Bool {
  return get_library().link(cursor);
}

auto Scene::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  return get_library().finalize(cursor);
}

auto Scene::Language::Monograph::get_name() const -> View::Bytes {
  return "Scene"_view;
}

auto Scene::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& child = get_library().resolve_context(route);
  if (!child.is<Invalid>()) {
    return child;
  }
  return Tetrodotoxin::Language::Monograph::resolve_context(route);
}

auto Scene::Language::Monograph::resolve_access(
    const Abstract& host,
    View::Bytes route) const -> const Abstract& {
  return get_library().resolve_access(host, route);
}

auto Scene::Language::Monograph::resolve_call(
    const Abstract& host,
    View::Bytes route) const -> const Abstract& {
  return get_library().resolve_call(host, route);
}
