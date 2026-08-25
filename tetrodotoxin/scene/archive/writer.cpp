// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/archive/writer.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/library/archive/writer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using Appender = Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>;

Scene::Archive::Writer::Writer(
    Tetrodotoxin::Language::Persistence::Profile profile) {
  Appender appender(bytes);
  appender << "TTSC"_view;
  appender << U16(2);
  appender << U8(profile);
  appender << U8(0);
}

auto Scene::Archive::Writer::encode(
    const Scene::Language::Monograph& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Dynamic::Bytes> {
  BAIL_IF(!monograph.is_finalized());
  auto child =
      Library::Archive::Writer::write(monograph.get_library(), profile);
  BAIL_IF(!child);

  Writer writer(profile);
  BAIL_IF(
      !writer.write(child->get_view()) ||
      monograph.get_signals().get_size() > U32(-1));
  writer.write(U32(monograph.get_signals().get_size()));
  for (const Reference<Scene::Language::Signal>& retained :
       monograph.get_signals()) {
    const Scene::Language::Signal& signal = retained.get();
    BAIL_IF(
        !writer.write(signal.get_documentation()) ||
        !writer.write(signal.get_name()));
    const auto& payload = signal.get_payload_reference();
    writer.write(U8(payload ? 1 : 0));
    if (payload) {
      auto encoded =
          Library::Archive::Writer::encode_type_reference(*payload, profile);
      BAIL_IF(!encoded || !writer.write(encoded->get_view()));
    }
  }

  Count lifecycle_count = 0;
  for (U8 role = 0; role <= U8(Scene::Language::Lifecycle::Release); role++) {
    lifecycle_count +=
        monograph.get_lifecycle(Scene::Language::Lifecycle(role)) ? 1 : 0;
  }
  writer.write(U8(lifecycle_count));
  for (U8 role = 0; role <= U8(Scene::Language::Lifecycle::Release); role++) {
    auto function = monograph.get_lifecycle(Scene::Language::Lifecycle(role));
    if (!function) {
      continue;
    }
    writer.write(role);
    BAIL_IF(!writer.write(function->get_name()));
  }
  return Data::take(writer.bytes);
}

auto Scene::Archive::Writer::write(U8 value) -> void {
  Appender(bytes) << value;
}

auto Scene::Archive::Writer::write(U16 value) -> void {
  Appender(bytes) << value;
}

auto Scene::Archive::Writer::write(U32 value) -> void {
  Appender(bytes) << value;
}

auto Scene::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Appender appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Scene::Archive::Writer::write(const Documentation& documentation) -> Bool {
  BAIL_IF(documentation.line_count() > U32(-1));
  write(U32(documentation.line_count()));
  for (Count index = 0; index < documentation.line_count(); index++) {
    BAIL_IF(!write(documentation.get_line(index)));
  }
  return True;
}
