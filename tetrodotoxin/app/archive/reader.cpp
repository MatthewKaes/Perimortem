// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/archive/reader.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/reader/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using LittleReader = Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>;

auto App::Archive::Reader::read(
    Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Abstract& dialect,
    const Documentation& documentation,
    Abstract& context) const -> Option<App::Language::Monograph&> {
  LittleReader reader(payload);
  View::Bytes magic = reader.read_bytes(4);
  U16 format = reader.read_u16();
  U8 encoded_profile = reader.read_u8();
  U8 runtime = reader.read_u8();
  U32 route_size = reader.read_u32();
  View::Bytes route = reader.read_bytes(route_size);
  U32 callable_size = reader.read_u32();
  View::Bytes callable = reader.read_bytes(callable_size);
  Bool valid = magic == "TTAP"_view && format == 1 &&
               encoded_profile == U8(profile) && runtime == 1 &&
               !route.is_empty() && !callable.is_empty() &&
               reader.get_location() == payload.get_size();
  if (!valid) {
    Diagnostics::Log::error(
        "App Archive payload failed Format 1 validation."_view);
    return {};
  }

  App::Language::Runtime& selected_runtime =
      App::Language::Runtime::create_synthetic(arena, documentation);
  App::Language::Program& selected_program =
      App::Language::Program::create_synthetic(
          arena, documentation, route, callable);
  return App::Language::Monograph::create(
      arena, dialect, documentation, context, selected_runtime,
      selected_program);
}
