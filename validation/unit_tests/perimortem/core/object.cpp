// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/object.h"

#include "validation/unit_test.hpp"

#include "perimortem/core/implementation.h"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/buffer.h"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/record.hpp"

using namespace Perimortem::Memory;
using namespace Validation;

static Harness CoreObject = {
  .name = "Core::Object and Memory::Dynamic::Record"_view,
};

class RaiiProbe {
 public:
  RaiiProbe(Count& destructor_count, Count value = 0)
      : destructor_count(destructor_count), value(value) {}

  ~RaiiProbe() { destructor_count++; }

  constexpr auto get_value() const -> Count { return value; }

 private:
  Count& destructor_count;
  Count value = 0;
};

static_assert(sizeof(Dynamic::Record<RaiiProbe>) == sizeof(U8*));

static Count native_finalizations = 0;

static auto finalize_native_object(U8*) -> void {
  native_finalizations++;
}

static constexpr perimortem_object_descriptor native_descriptor{
  .size = sizeof(U64),
  .alignment = alignof(U64),
  .finalize = finalize_native_object,
};

PERIMORTEM_UNIT_TEST(CoreObject, native_surface) {
  native_finalizations = 0;
  U8* object = perimortem_core_object_allocate(&native_descriptor);
  const perimortem_object_descriptor* descriptor =
      perimortem_core_object_descriptor(object);
  EXPECT_EQ(descriptor->size, Count(sizeof(U64)));
  EXPECT_EQ(descriptor->alignment, Count(alignof(U64)));
  EXPECT(descriptor->finalize == finalize_native_object);
  perimortem_core_object_retain(object);
  perimortem_core_object_release(object);
  EXPECT_EQ(native_finalizations, Count(0));
  perimortem_core_object_release(object);
  EXPECT_EQ(native_finalizations, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, empty_surface) {
  perimortem_core_object_retain({});
  perimortem_core_object_release({});
  EXPECT_EQ(perimortem_core_object_capacity({}), Count(0));
}

PERIMORTEM_UNIT_TEST(CoreObject, shared_buffer_access) {
  U64 initial = 0;
  U8* first = perimortem_core_object_reserve(
      nullptr, &native_descriptor, 3, sizeof(U64),
      reinterpret_cast<const U8*>(&initial));
  auto* first_values = reinterpret_cast<U64*>(first);
  first_values[0] = 42;

  U8* second = first;
  perimortem_core_object_retain(second);
  EXPECT_EQ(perimortem_core_object_reservations(first), Count(2));
  reinterpret_cast<U64*>(second)[0] = 7;
  EXPECT_EQ(reinterpret_cast<U64*>(first)[0], U64(7));

  second =
      perimortem_core_object_clone(second, &native_descriptor, sizeof(U64));
  EXPECT_EQ(perimortem_core_object_reservations(first), Count(1));
  EXPECT(first != second);
  reinterpret_cast<U64*>(second)[0] = 9;
  EXPECT_EQ(reinterpret_cast<U64*>(first)[0], U64(7));
  EXPECT_EQ(reinterpret_cast<U64*>(second)[0], U64(9));
  perimortem_core_object_release(first);
  perimortem_core_object_release(second);
}

PERIMORTEM_UNIT_TEST(CoreObject, empty_option_payload) {
  U8* empty = nullptr;
  Perimortem::Core::Option<U8*> selected(static_cast<U8*&&>(empty));

  EXPECT(selected);
  EXPECT(*selected == nullptr);
  EXPECT(sizeof(selected) > sizeof(U8*));
}

PERIMORTEM_UNIT_TEST(CoreObject, implementation_ownership) {
  static U8 projection_storage = 0;
  const auto* projection =
      reinterpret_cast<const perimortem_projection*>(&projection_storage);
  U8* object = perimortem_core_object_allocate(&native_descriptor);
  perimortem_implementation first = {};
  ASSERT(perimortem_core_implementation_retain(object, projection, &first));
  EXPECT_EQ(perimortem_core_object_reservations(object), Count(2));

  perimortem_implementation second = {};
  perimortem_core_implementation_copy(&first, &second);
  EXPECT_EQ(perimortem_core_object_reservations(object), Count(3));
  EXPECT(perimortem_core_implementation_is_valid(&second));

  perimortem_implementation moved = {};
  perimortem_core_implementation_move(&moved, &second);
  EXPECT(perimortem_core_implementation_is_empty(&second));
  EXPECT_EQ(moved.object, object);
  EXPECT_EQ(moved.projection, projection);

  perimortem_core_implementation_release(&first);
  perimortem_core_implementation_release(&moved);
  EXPECT_EQ(perimortem_core_object_reservations(object), Count(1));
  perimortem_core_object_release(object);
}

PERIMORTEM_UNIT_TEST(CoreObject, shared_lifetime) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> probe(destructor_count, 42);
    EXPECT_EQ(probe->get_value(), Count(42));

    {
      Dynamic::Record<RaiiProbe> second = probe;
      EXPECT_EQ(second->get_value(), Count(42));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 1);
    Dynamic::Record<RaiiProbe> second(destructor_count, 2);

    second = first;
    EXPECT_EQ(destructor_count, Count(1));
    EXPECT_EQ(second->get_value(), Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(CoreObject, assignment_reserves) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 3);
    Dynamic::Record<RaiiProbe> second = first;
    const Dynamic::Record<RaiiProbe>& alias = first;

    first = alias;
    second = first;
    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, move_assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 1);
    {
      Dynamic::Record<RaiiProbe> second(destructor_count, 2);

      first = static_cast<Dynamic::Record<RaiiProbe>&&>(second);
      EXPECT_EQ(first->get_value(), Count(2));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(first->get_value(), Count(2));
    EXPECT_EQ(destructor_count, Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(CoreObject, move_construction) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 3);
    {
      Dynamic::Record<RaiiProbe> second(
          static_cast<Dynamic::Record<RaiiProbe>&&>(first));
      EXPECT_EQ(second->get_value(), Count(3));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, map_owner) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Record<RaiiProbe>> values;

  {
    Dynamic::Record<RaiiProbe> probe(destructor_count, 7);
    values.insert(0, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  auto found = values.find(0);
  ASSERT(found);
  EXPECT_EQ((*found).value->get_value(), Count(7));

  values.remove(0);
  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, map_rehash) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Record<RaiiProbe>> values;
  for (Count i = 0; i < 16; i++) {
    Dynamic::Record<RaiiProbe> probe(destructor_count, i);
    values.insert(i, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  for (Count i = 0; i < 16; i++) {
    auto found = values.find(i);
    ASSERT(found);
    EXPECT_EQ((*found).value->get_value(), i);
  }

  values.clear();
  EXPECT_EQ(destructor_count, Count(16));
}
