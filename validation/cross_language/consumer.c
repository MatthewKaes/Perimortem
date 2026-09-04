// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cross_language/bridge.h"

static uint64_t C_SUPPORT_AUTHORITY;
static uint64_t C_CALLBACK_AUTHORITY;

#define ABI_HEADER(type)            \
  {                                 \
    .size = (uint32_t)sizeof(type), \
    .abi_major = TTX_ABI_MAJOR,     \
    .abi_minor = TTX_ABI_MINOR,     \
  }

#define OWNER_FROM_OPERATIONS(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

struct enumerable_capture {
  ttx_enumerable_result_ops operations;
  uint8_t answered;
  uint8_t satisfied;
  ttx_enumerable enumerable;
};

struct first_frame {
  ttx_layout_entry_sink_ops operations;
  uint64_t count;
  uint8_t completed;
  ttx_abstract first;
};

enum consume_phase {
  CONSUME_BUILD_INPUT,
  CONSUME_INVOKE,
};

struct consume_frame {
  ttx_interface_sink_ops interface_operations;
  ttx_pack_result_ops pack_operations;
  ttx_abstract candidate;
  ttx_abstract requirement;
  ttx_abstract operation;
  ttx_pack input_pack;
  ttx_context context;
  ttx_interface_relation relation;
  enum consume_phase phase;
  ttx_test_receipt receipt;
};

static const ttx_test_bytes_terminal_ops bytes_terminal_operations;

struct provider_projection {
  ttx_test_bytes_sink_ops operations;
  ttx_test_bytes_sink result;
  uint8_t answered;
  uint8_t projected;
};

struct empty_discovery_state {
  ttx_concept_sink_ops operations;
  ttx_abstract requirement;
  ttx_abstract operation;
  ttx_context context;
  ttx_test_discovery result;
};

static ttx_test_bytes_terminal* bytes_providers;
static uint64_t bytes_provider_count;

static void* grow(void* data, uint64_t count, size_t element_size) {
  if (count > SIZE_MAX / element_size) {
    return 0;
  }
  return realloc(data, (size_t)count * element_size);
}

static void ensure_authorities(void) {
  if (C_SUPPORT_AUTHORITY == 0) {
    C_SUPPORT_AUTHORITY = ttx_authority_create();
    C_CALLBACK_AUTHORITY = ttx_authority_create();
  }
}

static struct enumerable_capture* enumerable_capture(
    ttx_enumerable_result self) {
  if (self.operations == 0) {
    return 0;
  }
  return OWNER_FROM_OPERATIONS(
      self.operations, struct enumerable_capture, operations);
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  struct enumerable_capture* capture = enumerable_capture(self);
  if (capture != 0) {
    capture->answered = 1;
    capture->satisfied = 0;
  }
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  struct enumerable_capture* capture = enumerable_capture(self);
  if (capture != 0) {
    capture->answered = 1;
    capture->satisfied = 1;
    capture->enumerable = enumerable;
  }
}

static uint8_t query_enumerable(ttx_layout layout, ttx_enumerable* enumerable) {
  struct enumerable_capture capture = {
    .operations =
        {
          .header = ABI_HEADER(ttx_enumerable_result_ops),
          .rejected = enumerable_rejected,
          .satisfied = enumerable_satisfied,
        },
  };
  const ttx_enumerable_result result = {
    .operations = &capture.operations,
    .owner = C_CALLBACK_AUTHORITY,
    .value = 1,
  };
  if (layout.operations == 0) {
    return 0;
  }
  layout.operations->enumerable(layout, result);
  if (capture.answered && capture.satisfied) {
    *enumerable = capture.enumerable;
    return 1;
  }
  return 0;
}

static struct first_frame* first_frame(ttx_layout_entry_sink self) {
  if (self.operations == 0) {
    return 0;
  }
  return OWNER_FROM_OPERATIONS(self.operations, struct first_frame, operations);
}

static void TTX_CALL first_entry(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  struct first_frame* frame = first_frame(self);
  (void)path;
  if (frame != 0) {
    if (frame->count == 0) {
      frame->first = producer;
    }
    ++frame->count;
  }
}

static void TTX_CALL first_completed(ttx_layout_entry_sink self) {
  struct first_frame* frame = first_frame(self);
  if (frame != 0) {
    frame->completed = 1;
  }
}

static struct provider_projection* provider_projection(
    ttx_test_bytes_sink self) {
  if (self.operations == 0) {
    return 0;
  }
  return OWNER_FROM_OPERATIONS(
      self.operations, struct provider_projection, operations);
}

static void TTX_CALL provider_rejected(ttx_test_bytes_sink self) {
  struct provider_projection* projection = provider_projection(self);
  if (projection != 0) {
    projection->answered = 1;
  }
}

static void TTX_CALL
    provider_projected(ttx_test_bytes_sink self, ttx_borrowed_bytes value) {
  struct provider_projection* projection = provider_projection(self);
  if (projection != 0) {
    projection->answered = 1;
    projection->projected = 1;
    projection->result.operations->projected(projection->result, value);
  }
}

static void TTX_CALL bytes_terminal_project(
    ttx_test_bytes_terminal self,
    ttx_abstract producer,
    ttx_abstract requirement,
    ttx_test_bytes_sink result) {
  uint64_t index;
  (void)self;
  for (index = 0; index < bytes_provider_count; ++index) {
    struct provider_projection projection = {
      .operations =
          {
            .header = ABI_HEADER(ttx_test_bytes_sink_ops),
            .rejected = provider_rejected,
            .projected = provider_projected,
          },
      .result = result,
    };
    const ttx_test_bytes_sink callback = {
      .operations = &projection.operations,
      .owner = C_CALLBACK_AUTHORITY,
      .value = 1,
    };
    bytes_providers[index].operations->project(
        bytes_providers[index], producer, requirement, callback);
    if (projection.answered && projection.projected) {
      return;
    }
  }
  result.operations->rejected(result);
}

static struct consume_frame* consume_frame_from_pack(ttx_pack_result self) {
  if (self.operations == 0) {
    return 0;
  }
  return OWNER_FROM_OPERATIONS(
      self.operations, struct consume_frame, pack_operations);
}

static struct consume_frame* consume_frame_from_interface(
    ttx_interface_sink self) {
  if (self.operations == 0) {
    return 0;
  }
  return OWNER_FROM_OPERATIONS(
      self.operations, struct consume_frame, interface_operations);
}

static void TTX_CALL consume_pack_unknown(ttx_pack_result self) {
  struct consume_frame* frame = consume_frame_from_pack(self);
  if (frame != 0) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_UNKNOWN;
  }
}

static void TTX_CALL consume_pack_none(ttx_pack_result self) {
  struct consume_frame* frame = consume_frame_from_pack(self);
  if (frame != 0) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_REJECTED;
  }
}

static void TTX_CALL consume_pack_support_failed(
    ttx_pack_result self,
    ttx_pack_support_failure failure) {
  struct consume_frame* frame = consume_frame_from_pack(self);
  (void)failure;
  if (frame != 0) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
  }
}

static ttx_pack_result consume_pack_result(struct consume_frame* frame) {
  const ttx_pack_result result = {
    .operations = &frame->pack_operations,
    .owner = C_CALLBACK_AUTHORITY,
    .value = 1,
  };
  return result;
}

static void TTX_CALL consume_pack_packed(ttx_pack_result self, ttx_pack pack) {
  struct consume_frame* frame = consume_frame_from_pack(self);
  if (frame == 0) {
    return;
  }
  if (frame->phase == CONSUME_BUILD_INPUT) {
    ttx_interface_sink interface_result;
    frame->input_pack = pack;
    frame->receipt.arguments = pack;
    interface_result.operations = &frame->interface_operations;
    interface_result.owner = C_CALLBACK_AUTHORITY;
    interface_result.value = 1;
    frame->candidate.operations->interface(
        frame->candidate, frame->requirement, interface_result);
    return;
  }
  frame->receipt.result = pack;
  frame->receipt.outcome = frame->relation == TTX_INTERFACE_EQUIVALENT
                               ? TTX_TEST_CONSUMED_EQUIVALENT
                               : TTX_TEST_CONSUMED_SATISFIED;
  {
    ttx_enumerable enumerable;
    const ttx_layout fitted_layout = pack.operations->layout(pack);
    if (!query_enumerable(fitted_layout, &enumerable)) {
      frame->receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
      return;
    }
    frame->receipt.cardinality = enumerable.operations->cardinality(enumerable);
  }
}

static void TTX_CALL
    consume_interface_answer(ttx_interface_sink self, ttx_interface interface) {
  struct consume_frame* frame = consume_frame_from_interface(self);
  if (frame == 0 ||
      !ttx_abstract_same(
          interface.operations->requirement(interface), frame->requirement) ||
      !ttx_abstract_same(
          interface.operations->candidate(interface), frame->candidate)) {
    if (frame != 0) {
      frame->receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
    }
    return;
  }
  frame->relation = interface.operations->negotiate(interface);
  if (frame->relation == TTX_INTERFACE_UNKNOWN) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_UNKNOWN;
    return;
  }
  if (frame->relation == TTX_INTERFACE_REJECTED) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_REJECTED;
    return;
  }
  frame->phase = CONSUME_INVOKE;
  interface.operations->invoke(
      interface, frame->operation, frame->input_pack, frame->context,
      consume_pack_result(frame));
}

static const ttx_test_bytes_terminal_ops bytes_terminal_operations = {
  .header = ABI_HEADER(ttx_test_bytes_terminal_ops),
  .project = bytes_terminal_project,
};

ttx_test_bytes_terminal TTX_CALL ttx_test_create_bytes_terminal(void) {
  ensure_authorities();
  const ttx_test_bytes_terminal result = {
    .operations = &bytes_terminal_operations,
    .owner = C_SUPPORT_AUTHORITY,
    .value = 1,
  };
  return result;
}

void TTX_CALL ttx_test_bytes_terminal_add(ttx_test_bytes_terminal provider) {
  ttx_test_bytes_terminal* resized = grow(
      bytes_providers, bytes_provider_count + 1,
      sizeof(ttx_test_bytes_terminal));
  if (resized == 0 || provider.operations == 0) {
    return;
  }
  bytes_providers = resized;
  bytes_providers[bytes_provider_count++] = provider;
}

static void initialize_consume_frame(
    struct consume_frame* frame,
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_context context,
    enum consume_phase phase) {
  memset(frame, 0, sizeof(*frame));
  frame->interface_operations = (ttx_interface_sink_ops){
    .header = ABI_HEADER(ttx_interface_sink_ops),
    .answer = consume_interface_answer,
  };
  frame->pack_operations = (ttx_pack_result_ops){
    .header = ABI_HEADER(ttx_pack_result_ops),
    .unknown = consume_pack_unknown,
    .none = consume_pack_none,
    .packed = consume_pack_packed,
    .support_failed = consume_pack_support_failed,
  };
  frame->candidate = candidate;
  frame->requirement = requirement;
  frame->operation = operation;
  frame->context = context;
  frame->phase = phase;
  frame->receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
}

ttx_test_receipt TTX_CALL ttx_test_consume_pack(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context) {
  struct consume_frame frame;
  ttx_interface_sink interface_result;
  ttx_test_receipt receipt = {0};
  if (candidate.operations == 0 || requirement.operations == 0 ||
      operation.operations == 0 || input.operations == 0 ||
      context.operations == 0 ||
      context.operations->header.abi_major != TTX_ABI_MAJOR ||
      context.operations->header.size < sizeof(ttx_context_ops)) {
    receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
    return receipt;
  }
  initialize_consume_frame(
      &frame, candidate, requirement, operation, context, CONSUME_INVOKE);
  frame.input_pack = input;
  frame.receipt.arguments = input;
  interface_result.operations = &frame.interface_operations;
  interface_result.owner = C_CALLBACK_AUTHORITY;
  interface_result.value = 1;
  candidate.operations->interface(candidate, requirement, interface_result);
  return frame.receipt;
}

ttx_test_receipt TTX_CALL ttx_test_consume_empty(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_context context) {
  struct consume_frame frame;
  ttx_pack_result result;
  ttx_test_receipt receipt = {0};
  if (candidate.operations == 0 || requirement.operations == 0 ||
      operation.operations == 0 || context.operations == 0 ||
      context.operations->header.abi_major != TTX_ABI_MAJOR ||
      context.operations->header.size < sizeof(ttx_context_ops)) {
    receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
    return receipt;
  }
  initialize_consume_frame(
      &frame, candidate, requirement, operation, context, CONSUME_BUILD_INPUT);
  result = consume_pack_result(&frame);
  context.operations->pack(context, ttx_empty_layout(), result);
  return frame.receipt;
}

static void TTX_CALL empty_discovery_item(
    ttx_concept_sink self,
    ttx_borrowed_bytes route,
    ttx_abstract candidate) {
  struct empty_discovery_state* discovery = OWNER_FROM_OPERATIONS(
      self.operations, struct empty_discovery_state, operations);
  const ttx_test_receipt receipt = ttx_test_consume_empty(
      candidate, discovery->requirement, discovery->operation,
      discovery->context);
  (void)route;
  ++discovery->result.visited;
  switch (receipt.outcome) {
  case TTX_TEST_CONSUMED_UNKNOWN:
    ++discovery->result.unknown;
    break;
  case TTX_TEST_CONSUMED_REJECTED:
    ++discovery->result.rejected;
    break;
  case TTX_TEST_CONSUMED_SATISFIED:
    ++discovery->result.satisfied;
    break;
  case TTX_TEST_CONSUMED_EQUIVALENT:
    ++discovery->result.equivalent;
    break;
  case TTX_TEST_CONSUMED_SUPPORT_FAILED:
    ++discovery->result.support_failed;
    break;
  }
}

static void TTX_CALL empty_discovery_completed(ttx_concept_sink self) {
  (void)self;
}

ttx_test_discovery TTX_CALL ttx_test_discover_empty(
    ttx_abstract root,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_context context) {
  struct empty_discovery_state discovery = {
    .operations =
        {
          .header = ABI_HEADER(ttx_concept_sink_ops),
          .item = empty_discovery_item,
          .completed = empty_discovery_completed,
        },
    .requirement = requirement,
    .operation = operation,
    .context = context,
  };
  const ttx_concept_sink visitor = {
    .operations = &discovery.operations,
    .owner = C_CALLBACK_AUTHORITY,
    .value = 1,
  };
  root.operations->visit_concepts(root, visitor);
  return discovery.result;
}

uint64_t TTX_CALL ttx_test_pack_cardinality(ttx_pack pack) {
  ttx_enumerable enumerable;
  const ttx_layout layout = pack.operations->layout(pack);
  return query_enumerable(layout, &enumerable)
             ? enumerable.operations->cardinality(enumerable)
             : UINT64_MAX;
}

ttx_abstract TTX_CALL ttx_test_pack_first(ttx_pack pack) {
  ttx_enumerable enumerable;
  struct first_frame frame = {
    .operations =
        {
          .header = ABI_HEADER(ttx_layout_entry_sink_ops),
          .entry = first_entry,
          .completed = first_completed,
        },
  };
  const ttx_layout layout = pack.operations->layout(pack);
  if (!query_enumerable(layout, &enumerable)) {
    return ttx_unknown();
  }
  const ttx_layout_entry_sink visitor = {
    .operations = &frame.operations,
    .owner = C_CALLBACK_AUTHORITY,
    .value = 1,
  };
  enumerable.operations->visit(enumerable, visitor);
  if (!frame.completed ||
      frame.count != enumerable.operations->cardinality(enumerable)) {
    return ttx_unknown();
  }
  return frame.count == 0 ? ttx_none() : frame.first;
}

#undef OWNER_FROM_OPERATIONS
