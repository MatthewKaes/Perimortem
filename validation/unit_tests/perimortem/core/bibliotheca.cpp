// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/bibliotheca.h"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/thread/worker.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreBibliotheca = {
  .name = "Core::Bibliotheca"_view,
};

PERIMORTEM_UNIT_TEST(CoreBibliotheca, size_class_reuse) {
  const perimortem_count memory_before =
      perimortem_bibliotheca_allocated_memory();
  const struct perimortem_bibliotheca_allocation first =
      perimortem_bibliotheca_check_out(1);

  EXPECT(first.ptr != nullptr);
  EXPECT_EQ(first.capacity, perimortem_count(64));
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(first.ptr) %
          PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT,
      uintptr_t(0));
  EXPECT_EQ(
      perimortem_bibliotheca_reservation_count(first.ptr), perimortem_count(1));
  EXPECT_EQ(perimortem_bibliotheca_reserve(first.ptr), perimortem_count(2));
  EXPECT_EQ(perimortem_bibliotheca_remit(first.ptr), perimortem_count(1));
  EXPECT_EQ(perimortem_bibliotheca_remit(first.ptr), perimortem_count(0));
  EXPECT_EQ(perimortem_bibliotheca_allocated_memory(), memory_before);

  const struct perimortem_bibliotheca_allocation reused =
      perimortem_bibliotheca_check_out(1);
  EXPECT(reused.ptr == first.ptr);
  EXPECT_EQ(reused.capacity, first.capacity);
  perimortem_bibliotheca_remit(reused.ptr);
}

static U8 cleanup_order[2];
static Count cleanup_count = 0;

static auto record_first_cleanup() -> void {
  cleanup_order[cleanup_count++] = 1;
}

static auto record_second_cleanup() -> void {
  cleanup_order[cleanup_count++] = 2;
}

static auto register_worker_cleanup(View::Bytes) -> void {
  perimortem_core_cleanup_register(record_first_cleanup);
  perimortem_core_cleanup_register(record_second_cleanup);
}

PERIMORTEM_UNIT_TEST(CoreBibliotheca, worker_cleanup_order) {
  cleanup_count = 0;
  cleanup_order[0] = 0;
  cleanup_order[1] = 0;

  Thread::Worker worker =
      Thread::Worker::start("cleanup"_view, register_worker_cleanup);
  worker.join();

  ASSERT_EQ(cleanup_count, Count(2));
  EXPECT_EQ(cleanup_order[0], U8(2));
  EXPECT_EQ(cleanup_order[1], U8(1));
}
