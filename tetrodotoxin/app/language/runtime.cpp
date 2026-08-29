// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/runtime.hpp"

#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::App;

auto Language::Runtime::create_authored(
    Allocator::Arena& arena,
    const Documentation& documentation,
    Profile profile,
    Ttx::Lexical::Anchor anchor) -> Runtime& {
  return arena.construct_from<Runtime>(
      [&]() { return Runtime(documentation, profile, anchor, {}); });
}

auto Language::Runtime::create_windowed(
    Allocator::Arena& arena,
    const Documentation& documentation,
    Ttx::Lexical::Anchor anchor,
    Option<View::Bytes> title,
    Option<View::Bytes> icon_route,
    Option<const Tetrodotoxin::Language::Resource&> icon,
    Option<U32> width,
    Option<U32> height,
    Option<Bool> resizable) -> Runtime& {
  Option<const Tetrodotoxin::Language::Resource*> icon_reference;
  if (icon) {
    icon_reference = &*icon;
  }

  auto& settings = arena.construct<Windowed>(
      title, icon_route, icon_reference, width, height, resizable);
  return arena.construct_from<Runtime>([&]() {
    return Runtime(documentation, Profile::Windowed, anchor, settings);
  });
}

auto Language::Runtime::create_synthetic(
    Allocator::Arena& arena,
    const Documentation& documentation) -> Runtime& {
  return create_authored(
      arena, documentation, Profile::Terminal,
      Ttx::Lexical::Anchor::create({}));
}

auto Language::Runtime::get_name() const -> View::Bytes {
  switch (profile) {
  case Profile::Terminal:
    return "Terminal"_view;
  case Profile::Headless:
    return "Headless"_view;
  case Profile::Windowed:
    return "Windowed"_view;
  }
  return "Runtime"_view;
}

auto Language::Runtime::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
