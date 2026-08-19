#include <stddef.h>
#include <stdint.h>

#ifndef TTX_LLVM_HEADER
#define TTX_LLVM_HEADER "Runtime/c_abi.h"
#endif

#include TTX_LLVM_HEADER

_Static_assert(sizeof(ttx_Pair) == 16, "Pair must use its SysV carrier");
_Static_assert(offsetof(ttx_Pair, left) == 0, "Pair.left offset changed");
_Static_assert(offsetof(ttx_Pair, right) == 8, "Pair.right offset changed");
_Static_assert(sizeof(ttx_Large) == 24, "Large must use its SysV carrier");
_Static_assert(offsetof(ttx_Large, third) == 16, "Large.third offset changed");
_Static_assert(
    sizeof(ttx_results_llvm_5fresults) == 16,
    "multiple results must use their named carrier");
_Static_assert(
    sizeof(ttx_Option_5bUnsigned_5f64_5d) == 16,
    "Option must store its payload and selected state inline");
_Static_assert(
    offsetof(ttx_Option_5bUnsigned_5f64_5d, value) == 0,
    "Option payload offset changed");
_Static_assert(
    offsetof(ttx_Option_5bUnsigned_5f64_5d, set) == 8,
    "Option selected state offset changed");
_Static_assert(
    sizeof(ttx_Access_5bUnsigned_5f64_5d) == 16,
    "Access must use its pointer and size carrier");
_Static_assert(
    sizeof(ttx_Counter) == sizeof(void*),
    "Object must use one opaque pointer carrier");

static uint64_t dense_storage[] = {
  UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
static const uint64_t view_storage[] = {UINT64_C(5), UINT64_C(6)};
static uint64_t printed_value = UINT64_C(0);
static size_t printed_count = 0;

void ttx_system_print(uint64_t value) {
  printed_value = value;
  printed_count++;
}

ttx_Access_5bUnsigned_5f64_5d llvm_dense_access(void) {
  return (ttx_Access_5bUnsigned_5f64_5d){
    .data = dense_storage,
    .size = sizeof(dense_storage) / sizeof(dense_storage[0]),
  };
}

ttx_View_5bUnsigned_5f64_5d llvm_dense_view(void) {
  return (ttx_View_5bUnsigned_5f64_5d){
    .data = view_storage,
    .size = sizeof(view_storage) / sizeof(view_storage[0]),
  };
}

int run_runtime_integration(void) {
  ttx_runtime();
  const uint64_t object_static_value = llvm_object_static_value();
  const uint64_t object_behavior = llvm_object_behavior();
  const ttx_Pair pair = llvm_pair();
  const ttx_Large large = llvm_large();
  const ttx_results_llvm_5fresults results = llvm_results();
  const ttx_Option_5bUnsigned_5f64_5d absent = llvm_option(
      (ttx_Option_5bUnsigned_5f64_5d){.value = UINT64_MAX, .set = false});
  const ttx_Option_5bUnsigned_5f64_5d present = llvm_option(
      (ttx_Option_5bUnsigned_5f64_5d){.value = UINT64_C(37), .set = true});
  const uint64_t access_total = llvm_access_write(0);
  ttx_Counter object = llvm_object_create();
  if (object == NULL) {
    return 1;
  }
  const uint64_t initial_object = llvm_object_read(object);
  const ttx_results_llvm_5fobject_5fresults object_results =
      llvm_object_results(object);
  const int object_results_valid = object_results.optional.set &&
                                   object_results.optional.value == object &&
                                   object_results.count == UINT64_C(7);
  if (object_results.optional.set) {
    perimortem_dynamic_object_release(object_results.optional.value);
  }
  perimortem_dynamic_object_retain(object);
  perimortem_dynamic_object_release(object);
  const uint64_t changed_object = llvm_object_add(object, UINT64_C(5));
  const uint64_t read_object = llvm_object_read(object);
  perimortem_dynamic_object_release(object);
  if (pair.left != UINT64_C(9) || pair.right != UINT64_C(4) ||
      large.first != UINT64_C(4) || large.second != UINT64_C(5) ||
      large.third != UINT64_C(6) || llvm_large_sum(large) != UINT64_C(15) ||
      TTX_FUNC_small_static(UINT8_C(9), true) != UINT8_C(9) ||
      results.left != UINT64_C(7) || !results.active || absent.set ||
      !present.set || present.value != UINT64_C(37) ||
      llvm_nonobject() != UINT64_C(21) || access_total != UINT64_C(91)) {
    return 1;
  }
  if (initial_object != UINT64_C(7) || changed_object != UINT64_C(12) ||
      read_object != UINT64_C(12) || !object_results_valid) {
    return 2;
  }
  if (object_static_value != UINT64_C(2)) {
    return 3;
  }
  if (object_behavior != UINT64_C(44)) {
    return 4;
  }
  if (llvm_borrow_iteration() != UINT64_C(197) || printed_count != 1 ||
      printed_value != UINT64_C(21) || dense_storage[0] != UINT64_C(1) ||
      dense_storage[1] != UINT64_C(20) || dense_storage[2] != UINT64_C(37) ||
      dense_storage[3] != UINT64_C(47)) {
    return 5;
  }
  if (llvm_enumeration_iteration() != UINT64_C(22)) {
    return 6;
  }
  return 0;
}

#ifdef TTX_STANDALONE_INTEGRATION
int main(void) {
  return run_runtime_integration();
}
#endif
