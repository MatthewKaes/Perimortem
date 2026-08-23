// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>
#include <stdint.h>

#ifndef TTX_LLVM_HEADER
#define TTX_LLVM_HEADER "Runtime/c_abi.h"
#endif

#include TTX_LLVM_HEADER

_Static_assert(
    sizeof(ttx_runtime_Pair) == 16,
    "Pair must use its SysV carrier");
_Static_assert(
    offsetof(ttx_runtime_Pair, left) == 0,
    "Pair.left offset changed");
_Static_assert(
    offsetof(ttx_runtime_Pair, right) == 8,
    "Pair.right offset changed");
_Static_assert(
    sizeof(ttx_runtime_Large) == 24,
    "Large must use its SysV carrier");
_Static_assert(
    offsetof(ttx_runtime_Large, third) == 16,
    "Large.third offset changed");
_Static_assert(
    sizeof(ttx_results_llvm_5fresults) == 16,
    "multiple results must use their named carrier");
_Static_assert(
    sizeof(ttx_runtime_Option_5bU64_5d) == 16,
    "Option must store its payload and selected state inline");
_Static_assert(
    offsetof(ttx_runtime_Option_5bU64_5d, value) == 0,
    "Option payload offset changed");
_Static_assert(
    offsetof(ttx_runtime_Option_5bU64_5d, set) == 8,
    "Option selected state offset changed");
_Static_assert(
    sizeof(ttx_runtime_Result_5bU64_2c_20Bool_5d) == 16,
    "Result must store one inline alternative and selected state");
_Static_assert(
    offsetof(
        ttx_runtime_Result_5bU64_2c_20Bool_5d, value_selected) == 8,
    "Result selected state offset changed");
_Static_assert(
    sizeof(ttx_runtime_Result_5bCounter_2c_20Bool_5d) == 16,
    "Object Result must store one handle and selected state inline");
_Static_assert(
    sizeof(ttx_runtime_Result_5bBool_2c_20Counter_5d) == 16,
    "Object error Result must store one handle and selected state inline");
_Static_assert(
    sizeof(ttx_runtime_Result_5bBool_2c_20Large_5d) == 32,
    "Wide Result must align its largest union alternative");
_Static_assert(
    offsetof(ttx_runtime_Result_5bBool_2c_20Large_5d, value_selected) == 24,
    "Wide Result selected state offset changed");
_Static_assert(
    sizeof(ttx_runtime_Access_5bU64_5d) == 16,
    "Access must use its pointer and size carrier");
_Static_assert(
    sizeof(ttx_runtime_Counter) == sizeof(void*),
    "Object must use one opaque pointer carrier");
_Static_assert(
    sizeof(ttx_runtime_Option_5bCounter_5d) == sizeof(void*),
    "Option[Object] must use the null handle niche");

static uint64_t dense_storage[] = {
  UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
static const uint64_t view_storage[] = {UINT64_C(5), UINT64_C(6)};
static uint64_t printed_value = UINT64_C(0);
static size_t printed_count = 0;

void ttx_system_print(uint64_t value) {
  printed_value = value;
  printed_count++;
}

ttx_runtime_Access_5bU64_5d llvm_dense_access(void) {
  return (ttx_runtime_Access_5bU64_5d){
    .data = dense_storage,
    .size = sizeof(dense_storage) / sizeof(dense_storage[0]),
  };
}

ttx_runtime_View_5bU64_5d llvm_dense_view(void) {
  return (ttx_runtime_View_5bU64_5d){
    .data = view_storage,
    .size = sizeof(view_storage) / sizeof(view_storage[0]),
  };
}

uint64_t llvm_object_identity(ttx_runtime_Object_5bU8_5d value) {
  return (uint64_t)(uintptr_t)value;
}

int run_runtime_integration(void) {
  ttx_runtime();
  const uint64_t object_static_value = llvm_object_static_value();
  const uint64_t object_behavior = llvm_object_behavior();
  const ttx_runtime_Pair pair = llvm_pair();
  const ttx_runtime_Large large = llvm_large();
  const ttx_results_llvm_5fresults results = llvm_results();
  const ttx_runtime_Option_5bU64_5d absent = llvm_option(
      (ttx_runtime_Option_5bU64_5d){
        .value = UINT64_MAX,
        .set = false,
      });
  const ttx_runtime_Option_5bU64_5d present = llvm_option(
      (ttx_runtime_Option_5bU64_5d){
        .value = UINT64_C(37),
        .set = true,
      });
  const ttx_runtime_Option_5bU64_5d stopped =
      llvm_bool_propagation(false);
  const ttx_runtime_Option_5bU64_5d continued =
      llvm_bool_propagation(true);
  const ttx_runtime_Result_5bU64_2c_20Bool_5d result_value =
      llvm_result_propagation(
          (ttx_runtime_Result_5bU64_2c_20Bool_5d){
        .value = UINT64_C(41),
        .value_selected = true,
      });
  const ttx_runtime_Result_5bU64_2c_20Bool_5d result_error =
      llvm_result_propagation(
          (ttx_runtime_Result_5bU64_2c_20Bool_5d){
        .error = true,
        .value_selected = false,
      });
  const ttx_runtime_Result_5bBool_2c_20Large_5d wide_error =
      llvm_result_wide_propagation(
          (ttx_runtime_Result_5bBool_2c_20Large_5d){
        .error =
            {
              .first = UINT64_C(4),
              .second = UINT64_C(5),
              .third = UINT64_C(6),
            },
        .value_selected = false,
      });
  const uint64_t access_total = llvm_access_write(0);
  ttx_runtime_Counter object = llvm_object_create();
  if (object == NULL) {
    return 1;
  }
  const uint64_t initial_object = llvm_object_read(object);
  const ttx_runtime_Result_5bCounter_2c_20Bool_5d object_value =
      llvm_result_object(object, false);
  const ttx_runtime_Result_5bCounter_2c_20Bool_5d object_error =
      llvm_result_object(object, true);
  const ttx_runtime_Result_5bCounter_2c_20Bool_5d propagated_object =
      llvm_result_object_propagation(
          (ttx_runtime_Result_5bCounter_2c_20Bool_5d){
        .value = object,
        .value_selected = true,
      });
  const ttx_runtime_Result_5bBool_2c_20Counter_5d propagated_object_error =
      llvm_result_error_object_propagation(
          (ttx_runtime_Result_5bBool_2c_20Counter_5d){
        .error = object,
        .value_selected = false,
      });
  const int object_result_valid =
      object_value.value_selected && object_value.value == object &&
      !object_error.value_selected && object_error.error &&
      propagated_object.value_selected && propagated_object.value == object &&
      !propagated_object_error.value_selected &&
      propagated_object_error.error == object;
  if (object_value.value_selected) {
    perimortem_core_object_release(object_value.value);
  }
  if (propagated_object.value_selected) {
    perimortem_core_object_release(propagated_object.value);
  }
  if (!propagated_object_error.value_selected) {
    perimortem_core_object_release(propagated_object_error.error);
  }
  const ttx_results_llvm_5fobject_5fresults object_results =
      llvm_object_results(object);
  const int object_results_valid =
      object_results.optional == object && object_results.count == UINT64_C(7);
  if (object_results.optional) {
    perimortem_core_object_release(object_results.optional);
  }
  perimortem_core_object_retain(object);
  perimortem_core_object_release(object);
  const uint64_t changed_object = llvm_object_add(object, UINT64_C(5));
  const uint64_t read_object = llvm_object_read(object);
  perimortem_core_object_release(object);
  if (pair.left != UINT64_C(9) || pair.right != UINT64_C(4) ||
      large.first != UINT64_C(4) || large.second != UINT64_C(5) ||
      large.third != UINT64_C(6) || llvm_large_sum(large) != UINT64_C(15) ||
      TTX_FUNC_small_static(UINT8_C(9), true) != UINT8_C(9) ||
      results.left != UINT64_C(7) || !results.active || absent.set ||
      !present.set || present.value != UINT64_C(37) || stopped.set ||
      !continued.set || continued.value != UINT64_C(31) ||
      !result_value.value_selected || result_value.value != UINT64_C(41) ||
      result_error.value_selected || !result_error.error ||
      wide_error.value_selected || wide_error.error.first != UINT64_C(4) ||
      wide_error.error.second != UINT64_C(5) ||
      wide_error.error.third != UINT64_C(6) ||
      llvm_result_error(result_value) || !llvm_result_error(result_error) ||
      llvm_nonobject() != UINT64_C(21) || access_total != UINT64_C(91) ||
      llvm_bytes_concat_size() != UINT64_C(5) ||
      llvm_bytes_api() != UINT64_C(777) ||
      llvm_object_storage() != UINT64_C(18)) {
    return 1;
  }
  if (initial_object != UINT64_C(7) || changed_object != UINT64_C(12) ||
      read_object != UINT64_C(12) || !object_results_valid ||
      !object_result_valid) {
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
