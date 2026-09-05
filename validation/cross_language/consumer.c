// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cross_language/bridge.h"

#define ABI_HEADER(type)            \
  {                                 \
    .size = (uint32_t)sizeof(type), \
    .abi_major = TTX_ABI_MAJOR,     \
    .abi_minor = TTX_ABI_MINOR,     \
  }

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

struct empty_discovery_state {
  ttx_concept_sink_ops operations;
  ttx_abstract requirement;
  ttx_abstract operation;
  ttx_context context;
  ttx_test_discovery result;
};

static struct enumerable_capture* enumerable_capture(
    ttx_enumerable_result self) {
  return (struct enumerable_capture*)self.self;
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
    .self = (ttx_enumerable_result_self*)&capture,
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
  return (struct first_frame*)self.self;
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

struct bytes_projection {
  ttx_test_bytes_sink result;
  ttx_abstract producer;
  ttx_abstract requirement;
  uint8_t* bytes;
  uint64_t size;
  uint64_t expected;
  uint8_t complete;
  uint8_t valid;
};

static void copy_bytes(ttx_bytes_sink self, ttx_borrowed_bytes bytes) {
  struct bytes_projection* p = (struct bytes_projection*)self.self;
  if (p->complete || bytes.size > p->expected - p->size ||
      (bytes.size && !bytes.data)) {
    p->valid = 0;
    return;
  }
  if (bytes.size) {
    memcpy(p->bytes + p->size, bytes.data, (size_t)bytes.size);
  }
  p->size += bytes.size;
}
static void bytes_complete(ttx_bytes_sink self) {
  struct bytes_projection* p = (struct bytes_projection*)self.self;
  if (p->complete) {
    p->valid = 0;
  }
  p->complete = 1;
}
static void no_bytes(ttx_bytes_result self) {
  (void)self;
}
static void project_bytes(ttx_bytes_result self, ttx_bytes bytes) {
  struct bytes_projection* p = (struct bytes_projection*)self.self;
  if (!ttx_abstract_same(bytes.operations->candidate(bytes), p->producer)) {
    return;
  }
  p->expected = bytes.operations->size(bytes);
  if (p->expected > SIZE_MAX) {
    return;
  }
  p->bytes = malloc((size_t)p->expected + (p->expected == 0));
  if (!p->bytes) {
    return;
  }
  const ttx_bytes_sink_ops ops = {
    .header = ABI_HEADER(ttx_bytes_sink_ops),
    .bytes = copy_bytes,
    .completed = bytes_complete};
  bytes.operations->visit(
      bytes,
      (ttx_bytes_sink){.operations = &ops, .self = (ttx_bytes_sink_self*)p});
}
static void bytes_proof(ttx_interface_sink self, ttx_interface witness) {
  struct bytes_projection* p = (struct bytes_projection*)self.self;
  const ttx_interface_relation relation =
      witness->operations->negotiate(witness);
  if (!ttx_abstract_same(
          witness->operations->candidate(witness), p->producer) ||
      !ttx_abstract_same(
          witness->operations->requirement(witness), p->requirement) ||
      (relation != TTX_INTERFACE_SATISFIED &&
       relation != TTX_INTERFACE_EQUIVALENT)) {
    return;
  }
  const ttx_bytes_result_ops ops = {
    .header = ABI_HEADER(ttx_bytes_result_ops),
    .unknown = no_bytes,
    .none = no_bytes,
    .resolved = project_bytes};
  p->producer->operations->resolve_bytes(
      p->producer, (ttx_bytes_result){
                     .operations = &ops, .self = (ttx_bytes_result_self*)p});
}
static void bytes_terminal_project(
    ttx_test_bytes_terminal self,
    ttx_abstract producer,
    ttx_abstract requirement,
    ttx_test_bytes_sink result) {
  (void)self;
  struct bytes_projection p = {
    .result = result,
    .producer = producer,
    .requirement = requirement,
    .valid = 1};
  const ttx_interface_sink_ops ops = {
    .header = ABI_HEADER(ttx_interface_sink_ops), .answer = bytes_proof};
  producer->operations->interface(
      producer, requirement,
      (ttx_interface_sink){
        .operations = &ops, .self = (ttx_interface_sink_self*)&p});
  if (p.valid && p.complete && p.size == p.expected) {
    result.operations->projected(result, (ttx_borrowed_bytes){p.bytes, p.size});
  } else {
    result.operations->rejected(result);
  }
  free(p.bytes);
}

static struct consume_frame* consume_frame_from_pack(ttx_pack_result self) {
  return (struct consume_frame*)self.self;
}

static struct consume_frame* consume_frame_from_interface(
    ttx_interface_sink self) {
  return (struct consume_frame*)self.self;
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
    .self = (ttx_pack_result_self*)frame,
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
    interface_result.self = (ttx_interface_sink_self*)frame;
    frame->candidate->operations->interface(
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
          interface->operations->requirement(interface), frame->requirement) ||
      !ttx_abstract_same(
          interface->operations->candidate(interface), frame->candidate)) {
    if (frame != 0) {
      frame->receipt.outcome = TTX_TEST_CONSUMED_SUPPORT_FAILED;
    }
    return;
  }
  frame->relation = interface->operations->negotiate(interface);
  if (frame->relation == TTX_INTERFACE_UNKNOWN) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_UNKNOWN;
    return;
  }
  if (frame->relation == TTX_INTERFACE_REJECTED) {
    frame->receipt.outcome = TTX_TEST_CONSUMED_REJECTED;
    return;
  }
  frame->phase = CONSUME_INVOKE;
  interface->operations->invoke(
      interface, frame->operation, frame->input_pack, frame->context,
      consume_pack_result(frame));
}

static const ttx_test_bytes_terminal_ops bytes_terminal_operations = {
  .header = ABI_HEADER(ttx_test_bytes_terminal_ops),
  .project = bytes_terminal_project,
};

ttx_test_bytes_terminal TTX_CALL ttx_test_create_bytes_terminal(void) {
  static uint8_t terminal;
  const ttx_test_bytes_terminal result = {
    .operations = &bytes_terminal_operations,
    .self = (ttx_test_bytes_terminal_self*)&terminal,
  };
  return result;
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
  if (candidate->operations == 0 || requirement->operations == 0 ||
      operation->operations == 0 || input.operations == 0 ||
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
  interface_result.self = (ttx_interface_sink_self*)&frame;
  candidate->operations->interface(candidate, requirement, interface_result);
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
  if (candidate->operations == 0 || requirement->operations == 0 ||
      operation->operations == 0 || context.operations == 0 ||
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
  struct empty_discovery_state* discovery =
      (struct empty_discovery_state*)self.self;
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
    .self = (ttx_concept_sink_self*)&discovery,
  };
  root->operations->visit_concepts(root, visitor);
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
    .self = (ttx_layout_entry_sink_self*)&frame,
  };
  enumerable.operations->visit(enumerable, visitor);
  if (!frame.completed ||
      frame.count != enumerable.operations->cardinality(enumerable)) {
    return ttx_unknown();
  }
  return frame.count == 0 ? ttx_none() : frame.first;
}
