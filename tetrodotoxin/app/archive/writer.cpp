// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/archive/writer.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/app/language/transition.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

static auto write_bytes(Dynamic::Bytes& output, View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);
  writer << U32(value.get_size());
  writer << value;
  return True;
}

auto App::Archive::Writer::write(
    const App::Language::Monograph& monograph) const -> Option<Dynamic::Bytes> {
  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);
  writer << "TTAP"_view;
  writer << U16(5);
  writer << U8(monograph.get_runtime().get_profile());

  auto windowed = monograph.get_runtime().get_windowed();
  U8 settings = 0;
  if (windowed) {
    settings |= windowed->get_title() ? U8(1 << 0) : U8(0);
    settings |= windowed->get_icon_route() ? U8(1 << 1) : U8(0);
    settings |= windowed->get_width() ? U8(1 << 2) : U8(0);
    settings |= windowed->get_height() ? U8(1 << 3) : U8(0);
    settings |= windowed->get_resizable() ? U8(1 << 4) : U8(0);
  }
  writer << settings;
  if (windowed) {
    if (auto title = windowed->get_title()) {
      BAIL_IF(!write_bytes(output, *title));
    }
    if (auto icon = windowed->get_icon_route()) {
      BAIL_IF(!write_bytes(output, *icon));
      BAIL_IF(!windowed->get_icon());
    }
    if (auto width = windowed->get_width()) {
      Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output) << *width;
    }
    if (auto height = windowed->get_height()) {
      Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
          << *height;
    }
    if (auto resizable = windowed->get_resizable()) {
      Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
          << U8(*resizable ? 1 : 0);
    }
  }

  auto program = monograph.get_program();
  auto scene = monograph.get_scene();
  BAIL_IF(Bool(program) == Bool(scene));
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
      << U8(program ? 0 : 1);
  if (program) {
    BAIL_IF(
        !write_bytes(output, program->get_route()) ||
        !write_bytes(output, program->get_callable_name()));
    return Data::take(output);
  }

  BAIL_IF(!write_bytes(output, scene->get_initial_route().get_spelling()));
  BAIL_IF(scene->get_transitions().get_size() > U32(-1));
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
      << U32(scene->get_transitions().get_size());
  for (const App::Language::Transition* retained : scene->get_transitions()) {
    const App::Language::Transition& transition = *retained;
    BAIL_IF(
        !write_bytes(output, transition.get_source_route().get_spelling()) ||
        !write_bytes(output, transition.get_signal_name()));
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
        << U8(transition.get_action());
    const auto& destination = transition.get_destination_route();
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(output)
        << U8(destination ? 1 : 0);
    if (destination) {
      BAIL_IF(!write_bytes(output, destination->get_spelling()));
    }
  }
  return Data::take(output);
}
