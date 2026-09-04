// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#define TTX_ABI_IMPLEMENTATION
#include "ttx/abi.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

enum {
  CANONICAL_OWNER = 1,
  CANONICAL_UNKNOWN = 1,
  CANONICAL_NONE = 2,
  CANONICAL_CONSTANT_REQUIREMENT = 3,
  CANONICAL_CALLABLE_REQUIREMENT = 4,
  CANONICAL_ROUTE_REQUIREMENT = 5,
  CANONICAL_FINITE_EXTENT_REQUIREMENT = 6,
  CANONICAL_EMPTY_LAYOUT = 1,
  CANONICAL_EMPTY_ENUMERABLE = 1,
  CANONICAL_EMPTY_SNAPSHOT = 1,
};

struct canonical_interface_state {
  // Interface answers are borrowed only during the result callback. Keeping
  // the dispatch table in that frame gives its operations direct typed state
  // without an allocator, an erased callback pointer, or hidden thread state.
  ttx_interface_ops operations;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
};

struct empty_fit_state {
  // Empty fitting needs one nested observation of the source Layout. Its
  // callback frame stays on the requesting stack because the Layout cannot
  // retain the result handle after enumerable returns.
  ttx_enumerable_result_ops operations;
  uint8_t answered;
  uint8_t enumerable;
  uint64_t cardinality;
};

static std::atomic<uint64_t> next_authority = CANONICAL_OWNER + 1;

struct canonical_operations {
  static const ttx_abstract_ops abstract;
  static const ttx_documentation_ops documentation;
  static const ttx_layout_ops layout;
  static const ttx_enumerable_ops enumerable;
  static const ttx_layout_snapshot_ops snapshot;
};

static ttx_abstract canonical_abstract(uint64_t value) {
  const ttx_abstract result = {
    .operations = &canonical_operations::abstract,
    .owner = CANONICAL_OWNER,
    .value = value,
  };
  return result;
}

static ttx_documentation empty_documentation(void) {
  const ttx_documentation result = {
    .operations = &canonical_operations::documentation,
    .owner = CANONICAL_OWNER,
    .value = 1,
  };
  return result;
}

static ttx_layout empty_layout(void) {
  const ttx_layout result = {
    .operations = &canonical_operations::layout,
    .owner = CANONICAL_OWNER,
    .value = CANONICAL_EMPTY_LAYOUT,
  };
  return result;
}

static ttx_enumerable empty_enumerable(void) {
  const ttx_enumerable result = {
    .operations = &canonical_operations::enumerable,
    .owner = CANONICAL_OWNER,
    .value = CANONICAL_EMPTY_ENUMERABLE,
  };
  return result;
}

static ttx_layout_snapshot empty_snapshot(void) {
  const ttx_layout_snapshot result = {
    .operations = &canonical_operations::snapshot,
    .owner = CANONICAL_OWNER,
    .value = CANONICAL_EMPTY_SNAPSHOT,
  };
  return result;
}

static ttx_borrowed_bytes TTX_CALL canonical_name(ttx_abstract self) {
  static const uint8_t unknown_name[] = "Unknown";
  static const uint8_t none_name[] = "None";
  static const uint8_t constant_name[] = "Constant";
  static const uint8_t callable_name[] = "Callable";
  static const uint8_t route_name[] = "Route";
  static const uint8_t finite_extent_name[] = "Extent";
  ttx_borrowed_bytes result = {};
  if (self.value == CANONICAL_UNKNOWN) {
    result.data = unknown_name;
    result.size = sizeof(unknown_name) - 1;
  } else if (self.value == CANONICAL_NONE) {
    result.data = none_name;
    result.size = sizeof(none_name) - 1;
  } else if (self.value == CANONICAL_CONSTANT_REQUIREMENT) {
    result.data = constant_name;
    result.size = sizeof(constant_name) - 1;
  } else if (self.value == CANONICAL_CALLABLE_REQUIREMENT) {
    result.data = callable_name;
    result.size = sizeof(callable_name) - 1;
  } else if (self.value == CANONICAL_ROUTE_REQUIREMENT) {
    result.data = route_name;
    result.size = sizeof(route_name) - 1;
  } else if (self.value == CANONICAL_FINITE_EXTENT_REQUIREMENT) {
    result.data = finite_extent_name;
    result.size = sizeof(finite_extent_name) - 1;
  }
  return result;
}

static ttx_documentation TTX_CALL canonical_documentation(ttx_abstract self) {
  (void)self;
  return empty_documentation();
}

static void TTX_CALL
    canonical_resolve(ttx_abstract self, ttx_abstract_sink result) {
  result.operations->answer(result, self);
}

static void TTX_CALL canonical_resolve_concept(
    ttx_abstract self,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  (void)route;
  result.operations->answer(
      result, self.value == CANONICAL_NONE
                  ? self
                  : canonical_abstract(CANONICAL_UNKNOWN));
}

static void TTX_CALL
    canonical_visit_concepts(ttx_abstract self, ttx_concept_sink result) {
  (void)self;
  result.operations->completed(result);
}

static struct canonical_interface_state* interface_state(ttx_interface self) {
  if (self.owner != CANONICAL_OWNER || self.operations == 0) {
    return 0;
  }

  // The borrowed operations pointer is also the private dispatch anchor for
  // this callback frame. Keeping operations first makes that recovery a C++
  // pointer interconversion without putting a native address into identity.
  static_assert(offsetof(canonical_interface_state, operations) == 0);
  return reinterpret_cast<canonical_interface_state*>(
      const_cast<ttx_interface_ops*>(self.operations));
}

static ttx_abstract TTX_CALL interface_requirement(ttx_interface self) {
  return interface_state(self)->requirement;
}

static ttx_abstract TTX_CALL interface_candidate(ttx_interface self) {
  return interface_state(self)->candidate;
}

static ttx_interface_relation TTX_CALL interface_negotiate(ttx_interface self) {
  return interface_state(self)->relation;
}

static void TTX_CALL interface_invoke(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  (void)self;
  (void)operation;
  (void)input;
  (void)context;
  result.operations->none(result);
}

static void TTX_CALL canonical_interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  struct canonical_interface_state state = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .requirement = interface_requirement,
          .candidate = interface_candidate,
          .negotiate = interface_negotiate,
          .invoke = interface_invoke,
        },
    .requirement = requirement,
    .candidate = self,
    .relation = TTX_INTERFACE_REJECTED,
  };
  ttx_interface interface;
  if (self.value == CANONICAL_UNKNOWN) {
    state.relation = TTX_INTERFACE_UNKNOWN;
  } else if (ttx_abstract_same(self, requirement)) {
    state.relation = TTX_INTERFACE_EQUIVALENT;
  } else if (
      self.value == CANONICAL_NONE &&
      ttx_abstract_same(requirement, ttx_constant_requirement())) {
    state.relation = TTX_INTERFACE_SATISFIED;
  }
  interface.operations = &state.operations;
  interface.owner = CANONICAL_OWNER;
  interface.value = 1;
  result.operations->answer(result, interface);
}

static void TTX_CALL
    canonical_resolve_domain(ttx_abstract self, ttx_domain_result result) {
  if (self.value == CANONICAL_UNKNOWN) {
    result.operations->unknown(result);
  } else {
    result.operations->none(result);
  }
}

static void TTX_CALL
    canonical_resolve_callable(ttx_abstract self, ttx_callable_result result) {
  if (self.value == CANONICAL_UNKNOWN) {
    result.operations->unknown(result);
  } else {
    result.operations->none(result);
  }
}

static void TTX_CALL
    canonical_resolve_route(ttx_abstract self, ttx_route_result result) {
  if (self.value == CANONICAL_UNKNOWN) {
    result.operations->unknown(result);
  } else {
    result.operations->none(result);
  }
}

static void TTX_CALL canonical_resolve_finite_extent(
    ttx_abstract self,
    ttx_finite_extent_result result) {
  if (self.value == CANONICAL_UNKNOWN) {
    result.operations->unknown(result);
  } else {
    result.operations->none(result);
  }
}

static uint64_t TTX_CALL documentation_size(ttx_documentation self) {
  (void)self;
  return 0;
}

static void TTX_CALL
    documentation_visit(ttx_documentation self, ttx_bytes_sink result) {
  (void)self;
  result.operations->completed(result);
}

static void TTX_CALL empty_fit_enumerable_rejected(ttx_enumerable_result self) {
  if (self.owner == CANONICAL_OWNER && self.operations != 0) {
    static_assert(offsetof(empty_fit_state, operations) == 0);
    auto* state = reinterpret_cast<empty_fit_state*>(
        const_cast<ttx_enumerable_result_ops*>(self.operations));
    state->answered = 1;
    state->enumerable = 0;
  }
}

static void TTX_CALL empty_fit_enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  if (self.owner == CANONICAL_OWNER && self.operations != 0) {
    static_assert(offsetof(empty_fit_state, operations) == 0);
    auto* state = reinterpret_cast<empty_fit_state*>(
        const_cast<ttx_enumerable_result_ops*>(self.operations));
    state->answered = 1;
    state->enumerable = 1;
    state->cardinality = enumerable.operations->cardinality(enumerable);
  }
}

static void TTX_CALL empty_fit(
    ttx_layout self,
    ttx_pack source,
    ttx_context context,
    ttx_pack_result result) {
  ttx_layout source_layout = source.operations->layout(source);
  ttx_enumerable_result enumerable_result;
  struct empty_fit_state state = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = empty_fit_enumerable_rejected,
          .satisfied = empty_fit_enumerable_satisfied,
        },
    .answered = 0,
    .enumerable = 0,
    .cardinality = 0,
  };
  (void)self;
  enumerable_result.operations = &state.operations;
  enumerable_result.owner = CANONICAL_OWNER;
  enumerable_result.value = 1;
  source_layout.operations->enumerable(source_layout, enumerable_result);
  if (!state.answered || !state.enumerable || state.cardinality != 0) {
    result.operations->none(result);
    return;
  }
  context.operations->pack(context, source_layout, result);
}

static void TTX_CALL
    empty_enumerable_query(ttx_layout self, ttx_enumerable_result result) {
  (void)self;
  result.operations->satisfied(result, empty_enumerable());
}

static void TTX_CALL empty_named(ttx_layout self, ttx_named_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    empty_snapshot_query(ttx_layout self, ttx_layout_snapshot_result result) {
  (void)self;
  result.operations->retained(result, empty_snapshot());
}

static void TTX_CALL empty_fluid(ttx_layout self, ttx_fluid_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    empty_value(ttx_layout self, ttx_value_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    empty_composite(ttx_layout self, ttx_composite_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    empty_ranged(ttx_layout self, ttx_ranged_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static ttx_layout TTX_CALL enumerable_layout(ttx_enumerable self) {
  (void)self;
  return empty_layout();
}

static uint64_t TTX_CALL enumerable_cardinality(ttx_enumerable self) {
  (void)self;
  return 0;
}

static void TTX_CALL
    enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result) {
  (void)self;
  result.operations->completed(result);
}

static ttx_layout TTX_CALL snapshot_layout(ttx_layout_snapshot self) {
  (void)self;
  return empty_layout();
}

static void TTX_CALL snapshot_release(ttx_layout_snapshot self) {
  (void)self;
}

const ttx_abstract_ops canonical_operations::abstract = {
  .header =
      {
        .size = sizeof(ttx_abstract_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .name = canonical_name,
  .documentation = canonical_documentation,
  .resolve = canonical_resolve,
  .resolve_concept = canonical_resolve_concept,
  .visit_concepts = canonical_visit_concepts,
  .interface = canonical_interface,
  .resolve_domain = canonical_resolve_domain,
  .resolve_callable = canonical_resolve_callable,
  .resolve_route = canonical_resolve_route,
  .resolve_finite_extent = canonical_resolve_finite_extent,
};

const ttx_documentation_ops canonical_operations::documentation = {
  .header =
      {
        .size = sizeof(ttx_documentation_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .size = documentation_size,
  .visit_bytes = documentation_visit,
};

const ttx_layout_ops canonical_operations::layout = {
  .header =
      {
        .size = sizeof(ttx_layout_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .fit = empty_fit,
  .enumerable = empty_enumerable_query,
  .named = empty_named,
  .snapshot = empty_snapshot_query,
  .fluid = empty_fluid,
  .value = empty_value,
  .composite = empty_composite,
  .ranged = empty_ranged,
};

const ttx_enumerable_ops canonical_operations::enumerable = {
  .header =
      {
        .size = sizeof(ttx_enumerable_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .layout = enumerable_layout,
  .cardinality = enumerable_cardinality,
  .visit = enumerable_visit,
};

const ttx_layout_snapshot_ops canonical_operations::snapshot = {
  .header =
      {
        .size = sizeof(ttx_layout_snapshot_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .layout = snapshot_layout,
  .release = snapshot_release,
};

ttx_abstract TTX_CALL ttx_unknown(void) {
  return canonical_abstract(CANONICAL_UNKNOWN);
}

ttx_abstract TTX_CALL ttx_none(void) {
  return canonical_abstract(CANONICAL_NONE);
}

ttx_abstract TTX_CALL ttx_constant_requirement(void) {
  return canonical_abstract(CANONICAL_CONSTANT_REQUIREMENT);
}

ttx_abstract TTX_CALL ttx_callable_requirement(void) {
  return canonical_abstract(CANONICAL_CALLABLE_REQUIREMENT);
}

ttx_abstract TTX_CALL ttx_route_requirement(void) {
  return canonical_abstract(CANONICAL_ROUTE_REQUIREMENT);
}

ttx_abstract TTX_CALL ttx_finite_extent_requirement(void) {
  return canonical_abstract(CANONICAL_FINITE_EXTENT_REQUIREMENT);
}

ttx_layout TTX_CALL ttx_empty_layout(void) {
  return empty_layout();
}

uint64_t TTX_CALL ttx_authority_create(void) {
  const uint64_t result =
      next_authority.fetch_add(1, std::memory_order_relaxed);
  if (result == 0 || result == UINT64_MAX) {
    std::abort();
  }
  return result;
}
