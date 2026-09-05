// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#define TTX_ABI_IMPLEMENTATION
#include "ttx/abi.h"

#include <cstddef>
#include <cstdint>

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/model/requirement.hpp"

struct empty_fit_state {
  // Empty fitting needs one nested observation of the source Layout. Its
  // callback frame stays on the requesting stack because the Layout cannot
  // retain the result handle after enumerable returns.
  ttx_enumerable_result_ops operations;
  uint8_t answered;
  uint8_t enumerable;
  uint64_t cardinality;
};

struct canonical_operations {
  static const ttx_layout_ops layout;
  static const ttx_enumerable_ops enumerable;
  static const ttx_layout_snapshot_ops snapshot;
};

static uint8_t empty_layout_state;
static uint8_t empty_enumerable_state;
static uint8_t empty_snapshot_state;

static ttx_layout empty_layout(void) {
  const ttx_layout result = {
    .operations = &canonical_operations::layout,
    .self = reinterpret_cast<ttx_layout_self*>(&empty_layout_state),
  };
  return result;
}

static ttx_enumerable empty_enumerable(void) {
  const ttx_enumerable result = {
    .operations = &canonical_operations::enumerable,
    .self = reinterpret_cast<ttx_enumerable_self*>(&empty_enumerable_state),
  };
  return result;
}

static ttx_layout_snapshot empty_snapshot(void) {
  const ttx_layout_snapshot result = {
    .operations = &canonical_operations::snapshot,
    .self = reinterpret_cast<ttx_layout_snapshot_self*>(&empty_snapshot_state),
  };
  return result;
}

static void TTX_CALL empty_fit_enumerable_rejected(ttx_enumerable_result self) {
  if (self.self != nullptr) {
    auto* state = reinterpret_cast<empty_fit_state*>(self.self);
    state->answered = 1;
    state->enumerable = 0;
  }
}

static void TTX_CALL empty_fit_enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  if (self.self != nullptr) {
    auto* state = reinterpret_cast<empty_fit_state*>(self.self);
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
  enumerable_result.self =
      reinterpret_cast<ttx_enumerable_result_self*>(&state);
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

static void TTX_CALL
    empty_reindexed(ttx_layout self, ttx_reindexed_layout_result result) {
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
  .reindexed = empty_reindexed,
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
  return Ttx::Concept::Unknown::get_unknown().get_handle();
}

ttx_abstract TTX_CALL ttx_none(void) {
  return Ttx::Concept::None::get_none().get_handle();
}

ttx_abstract TTX_CALL ttx_constant_requirement(void) {
  static constinit Ttx::Requirement requirement("Constant"_bytes);
  return requirement.get_abi();
}

ttx_abstract TTX_CALL ttx_addressable_requirement(void) {
  static constinit Ttx::Requirement requirement("Addressable"_bytes);
  return requirement.get_abi();
}

ttx_abstract TTX_CALL ttx_callable_requirement(void) {
  static constinit Ttx::Requirement requirement("Callable"_bytes);
  return requirement.get_abi();
}

ttx_abstract TTX_CALL ttx_route_requirement(void) {
  static constinit Ttx::Requirement requirement("Route"_bytes);
  return requirement.get_abi();
}

ttx_abstract TTX_CALL ttx_finite_extent_requirement(void) {
  static constinit Ttx::Requirement requirement("Extent"_bytes);
  return requirement.get_abi();
}

ttx_abstract TTX_CALL ttx_bytes_requirement(void) {
  static constinit Ttx::Requirement requirement("Bytes"_bytes);
  return requirement.get_abi();
}

ttx_layout TTX_CALL ttx_empty_layout(void) {
  return empty_layout();
}
