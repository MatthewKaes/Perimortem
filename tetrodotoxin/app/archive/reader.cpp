// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/archive/reader.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/app/language/program.hpp"
#include "tetrodotoxin/app/language/route.hpp"
#include "tetrodotoxin/app/language/runtime.hpp"
#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/app/language/transition.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/lexical/anchor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

auto App::Archive::Reader::read(
    Allocator::Arena& arena,
    View::Bytes payload,
    const Abstract& dialect,
    const Documentation& documentation,
    Abstract& context) const -> Option<App::Language::Monograph&> {
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(payload);
  View::Bytes magic = reader.read_bytes(4);
  U16 format = reader.read_u16();
  U8 runtime_value = reader.read_u8();
  U8 settings = reader.read_u8();
  auto valid_reader = [&]() {
    return reader.get_location() <= payload.get_size();
  };
  if (!valid_reader() || magic != "TTAP"_view || format != 5 ||
      runtime_value > U8(App::Language::Runtime::Profile::Windowed) ||
      runtime_value == 0 || (settings & U8(~0x1F)) != 0) {
    Diagnostics::Log::error(
        "App Archive payload failed Format 5 validation."_view);
    return {};
  }

  auto read_bytes = [&]() -> Option<View::Bytes> {
    U32 size = reader.read_u32();
    if (!valid_reader() || size > payload.get_size() - reader.get_location()) {
      return {};
    }
    return reader.read_bytes(size);
  };
  auto runtime_profile = App::Language::Runtime::Profile(runtime_value);
  Option<View::Bytes> title;
  Option<View::Bytes> icon_route;
  Option<const Tetrodotoxin::Language::Resource&> icon;
  Option<U32> width;
  Option<U32> height;
  Option<Bool> resizable;
  if ((settings & (1 << 0)) != 0) {
    title = read_bytes();
    BAIL_IF(!title);
    title = arena.proxy(*title);
  }
  if ((settings & (1 << 1)) != 0) {
    icon_route = read_bytes();
    BAIL_IF(!icon_route);
    icon_route = arena.proxy(*icon_route);
    const Abstract& selected = context.resolve_concept(*icon_route).resolve();
    icon = selected.select<Tetrodotoxin::Language::Resource>();
    BAIL_IF(!icon || selected.is<Unknown>());
  }
  if ((settings & (1 << 2)) != 0) {
    width = reader.read_u32();
  }
  if ((settings & (1 << 3)) != 0) {
    height = reader.read_u32();
  }
  if ((settings & (1 << 4)) != 0) {
    U8 value = reader.read_u8();
    BAIL_IF(value > 1);
    resizable = Bool(value == 1);
  }
  BAIL_IF(
      !valid_reader() ||
      (runtime_profile != App::Language::Runtime::Profile::Windowed &&
       settings != 0));

  App::Language::Runtime& runtime =
      runtime_profile == App::Language::Runtime::Profile::Windowed
          ? App::Language::Runtime::create_windowed(
                arena, documentation, Ttx::Lexical::Anchor::create({}), title,
                icon_route, icon, width, height, resizable)
          : App::Language::Runtime::create_authored(
                arena, documentation, runtime_profile,
                Ttx::Lexical::Anchor::create({}));

  U8 lifecycle = reader.read_u8();
  BAIL_IF(!valid_reader() || lifecycle > 1);
  if (lifecycle == 0) {
    auto route = read_bytes();
    auto callable = read_bytes();
    BAIL_IF(
        !route || route->is_empty() || !callable || callable->is_empty() ||
        !valid_reader() || reader.get_location() != payload.get_size());
    auto& program = App::Language::Program::create_synthetic(
        arena, documentation, *route, *callable);
    return App::Language::Monograph::create_program(
        arena, dialect, documentation, context, runtime, program);
  }

  auto initial = read_bytes();
  U32 transition_count = reader.read_u32();
  BAIL_IF(
      !initial || initial->is_empty() || !valid_reader() ||
      transition_count > payload.get_size());
  Managed::Vector<App::Language::Transition*> transitions(arena);
  for (Count index = 0; index < transition_count; index++) {
    auto source = read_bytes();
    auto signal = read_bytes();
    U8 action_value = reader.read_u8();
    U8 has_destination = reader.read_u8();
    BAIL_IF(
        !source || source->is_empty() || !signal || signal->is_empty() ||
        !valid_reader() ||
        action_value > U8(App::Language::Transition::Action::Exit) ||
        has_destination > 1);
    Option<App::Language::Route> destination;
    if (has_destination == 1) {
      auto destination_route = read_bytes();
      BAIL_IF(!destination_route || destination_route->is_empty());
      destination =
          App::Language::Route::create_restored(arena, *destination_route);
    }
    auto action = App::Language::Transition::Action(action_value);
    BAIL_IF(
        (action == App::Language::Transition::Action::Replace ||
         action == App::Language::Transition::Action::Push) !=
        bool(destination));
    auto& transition = App::Language::Transition::create_restored(
        arena, documentation,
        App::Language::Route::create_restored(arena, *source), *signal, action,
        destination);
    transitions.insert(&transition);
  }
  BAIL_IF(!valid_reader() || reader.get_location() != payload.get_size());
  auto& scene = App::Language::Scene::create_restored(
      arena, documentation,
      App::Language::Route::create_restored(arena, *initial),
      transitions.get_view());
  return App::Language::Monograph::create_scene(
      arena, dialect, documentation, context, runtime, scene);
}
