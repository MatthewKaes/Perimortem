// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/invocation.hpp"

#include "validation/unit_test.hpp"

#include "cross_language/bridge.h"
#include "ttx/query.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness PufferInvocation = {
  .name = "Puffer::Invocation"_view,
};

struct BytesCapture {
  ttx_bytes_sink_ops operations;
  std::vector<uint8_t> bytes;
  bool completed;
};

static auto bytes_capture(ttx_bytes_sink self) -> BytesCapture& {
  return *reinterpret_cast<BytesCapture*>(self.self);
}

static void TTX_CALL
    append_bytes(ttx_bytes_sink self, ttx_borrowed_bytes bytes) {
  auto& capture = bytes_capture(self);
  capture.bytes.insert(
      capture.bytes.end(), bytes.data, bytes.data + bytes.size);
}

static void TTX_CALL finish_bytes(ttx_bytes_sink self) {
  bytes_capture(self).completed = true;
}

static auto read_bytes(ttx_abstract source) -> std::vector<uint8_t> {
  const Ttx::BytesObservation observation = Ttx::resolve_bytes(source);
  if (observation.state != Ttx::Observation::Resolved) {
    return {};
  }
  BytesCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .bytes = append_bytes,
          .completed = finish_bytes,
        },
    .bytes = {},
    .completed = false,
  };
  observation.bytes.operations->visit(
      observation.bytes, {
                           .operations = &capture.operations,
                           .self = reinterpret_cast<ttx_bytes_sink_self*>(
                               &capture),
                         });
  return capture.completed ? capture.bytes : std::vector<uint8_t>();
}

PERIMORTEM_UNIT_TEST(PufferInvocation, raises_dialect_owned_arguments) {
  Puffer::Invocation invocation(
      "/project"_view, "/sdk"_view,
      std::vector<View::Bytes>{"profile=release"_view, "graph"_view});
  const auto& arguments = invocation.resolve_concept("arguments"_view);
  const Ttx::DomainObservation domain =
      Ttx::resolve_domain(arguments.get_handle());
  const ttx_context context = ttx_context_create();
  const Ttx::PackObservation pack = Ttx::pack(context, domain.layout);

  ASSERT(domain.state == Ttx::Observation::Resolved);
  ASSERT(pack.state == Ttx::PackObservationState::Packed);
  EXPECT_EQ(ttx_test_pack_cardinality(pack.pack), uint64_t(2));
  EXPECT(
      read_bytes(ttx_test_pack_first(pack.pack)) ==
      std::vector<uint8_t>(
          {'p', 'r', 'o', 'f', 'i', 'l', 'e', '=', 'r', 'e', 'l', 'e', 'a', 's',
           'e'}));
  EXPECT(
      read_bytes(
          invocation.resolve_concept("working_directory"_view).get_handle()) ==
      std::vector<uint8_t>({'/', 'p', 'r', 'o', 'j', 'e', 'c', 't'}));
  context.operations->release(context);
}
