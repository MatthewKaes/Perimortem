// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/archive/reader.hpp"

#include "perimortem/core/reader/binary.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"
#include "ttx/model/documentations/block.hpp"
#include "ttx/lexical/anchor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

auto Scene::Archive::Reader::open(View::Bytes payload) -> Option<Reader> {
  BAIL_IF(payload.get_size() < 8);
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(
      payload.slice(0, 8));
  View::Bytes magic = reader.read_bytes(4);
  U16 version = reader.read_u16();
  U16 flags = reader.read_u16();
  BAIL_IF(magic != "TTSC"_view || version != 3 || flags != 0);
  return Reader(payload.slice(8));
}

auto Scene::Archive::Reader::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    const Abstract& language,
    const Library::Dialect& library,
    Abstract& context) -> Option<Scene::Language::Monograph&> {
  auto opened = open(payload);
  BAIL_IF(!opened);
  auto child_payload = opened->read_bytes();
  BAIL_IF(!child_payload);

  auto child =
      Library::Archive::Reader::read(arena, *child_payload, library, context);
  BAIL_IF(!child);
  Option<Library::Language::Types::Object&> instance;
  for (Abstract* declaration : child->get_source().get_declarations()) {
    auto candidate = declaration->select<Library::Language::Types::Object>();
    if (candidate && candidate->get_name() == "Scene"_view) {
      BAIL_IF(instance);
      instance = *candidate;
    }
  }
  BAIL_IF(!instance);

  auto& monograph = Scene::Language::Monograph::create(
      arena, child->get_documentation(), language, context, *child, *instance);
  auto signal_count = opened->read_u32();
  BAIL_IF(!signal_count || Count(*signal_count) > opened->payload.get_size());
  for (Count index = 0; index < *signal_count; index++) {
    auto documentation = opened->read_documentation(arena);
    auto name = opened->read_bytes();
    auto has_payload = opened->read_u8();
    BAIL_IF(
        !documentation || !name || name->is_empty() || !has_payload ||
        *has_payload > 1);

    Option<Library::Language::TypeReference> payload_reference;
    if (*has_payload == 1) {
      auto reference_payload = opened->read_bytes();
      BAIL_IF(!reference_payload);
      payload_reference = Library::Archive::Reader::restore_type_reference(
          arena, *reference_payload, *instance);
      BAIL_IF(!payload_reference);
    }
    auto& signal = Scene::Language::Signal::create_restored(
        arena, *documentation, *name, payload_reference);
    BAIL_IF(!monograph.retain_restored_signal(signal));
  }

  auto lifecycle_count = opened->read_u8();
  BAIL_IF(!lifecycle_count || *lifecycle_count > 5);
  for (Count index = 0; index < *lifecycle_count; index++) {
    auto role = opened->read_u8();
    auto name = opened->read_bytes();
    BAIL_IF(
        !role || *role > U8(Scene::Language::Lifecycle::Release) || !name ||
        name->is_empty());

    Option<Library::Language::Function&> function;
    for (Abstract* declaration : instance->get_declarations()) {
      auto candidate = declaration->select<Library::Language::Function>();
      if (candidate && candidate->get_name() == *name) {
        BAIL_IF(function);
        function = *candidate;
      }
    }
    BAIL_IF(
        !function || !monograph.retain_restored_lifecycle(
                         Scene::Language::Lifecycle(*role), *function));
  }
  BAIL_IF(!opened->is_complete());
  return monograph;
}

auto Scene::Archive::Reader::take(Count size) -> Option<View::Bytes> {
  BAIL_IF(
      location > payload.get_size() || size > payload.get_size() - location);
  View::Bytes selected = payload.slice(location, size);
  location += size;
  return selected;
}

auto Scene::Archive::Reader::read_u8() -> Option<U8> {
  auto selected = take(sizeof(U8));
  return selected
             ? Option<U8>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_u8())
             : Option<U8>();
}

auto Scene::Archive::Reader::read_u32() -> Option<U32> {
  auto selected = take(sizeof(U32));
  return selected
             ? Option<U32>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_u32())
             : Option<U32>();
}

auto Scene::Archive::Reader::read_bytes() -> Option<View::Bytes> {
  auto size = read_u32();
  BAIL_IF(!size);
  return take(*size);
}

auto Scene::Archive::Reader::read_documentation(Allocator::Arena& arena)
    -> Option<const Documentation&> {
  auto count = read_u32();
  BAIL_IF(!count || Count(*count) > payload.get_size());
  auto lines = arena.reserve<View::Bytes>(*count);
  for (Count index = 0; index < *count; index++) {
    auto line = read_bytes();
    BAIL_IF(!line);
    lines.get_data()[index] = arena.proxy(*line);
  }
  return arena.construct<Ttx::Documentations::Block>(
      View::Vector<View::Bytes>(lines.get_data(), lines.get_size()));
}
