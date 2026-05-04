// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/math/sort.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/utility/null_terminated.hpp"

#include <stdlib.h>

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;

using namespace Validation;

namespace Perimortem::Memory::Dynamic {
auto operator>(const Bytes& lhs, const Bytes& rhs) -> Bool {
  if (lhs.get_size() != rhs.get_size()) {
    return lhs.get_size() > rhs.get_size();
  }

  auto data = lhs.get_view().get_data();
  auto comp = rhs.get_view().get_data();
  for (Count i = 0; i < lhs.get_size(); i++) {
    if (data[i] != comp[i]) {
      return data[i] > comp[i];
    }
  }

  return false;
}
}  // namespace Perimortem::Memory::Dynamic

Test::Harness MathSort = {.name = "Math::Sort"};

PERIMORTEM_UNIT_TEST(MathSort, simple_sort) {
  constexpr Int test_const[] = {9, 8, 1, 4, 3, 7};
  Int test[] = {9, 8, 1, 4, 3, 7};

  constexpr auto sorted_const = sort(test_const);
  auto sorted = sort(Access::Vector(test));

  EXPECT_EQ(sorted[0], 1);
  EXPECT_EQ(sorted[1], 3);
  EXPECT_EQ(sorted[2], 4);
  EXPECT_EQ(sorted[3], 7);
  EXPECT_EQ(sorted[4], 8);
  EXPECT_EQ(sorted[5], 9);

  EXPECT_EQ(sorted[0], sorted_const[0]);
  EXPECT_EQ(sorted[1], sorted_const[1]);
  EXPECT_EQ(sorted[2], sorted_const[2]);
  EXPECT_EQ(sorted[3], sorted_const[3]);
  EXPECT_EQ(sorted[4], sorted_const[4]);
  EXPECT_EQ(sorted[5], sorted_const[5]);
}

PERIMORTEM_UNIT_TEST(MathSort, empty_sort) {
  constexpr auto sorted_const = sort(Access::Vector<Int>());
  auto sorted = sort(Access::Vector<Int>());

  EXPECT_EQ(sorted.get_size() , 0);
  EXPECT_EQ(sorted_const.get_size() , 0);
}

PERIMORTEM_UNIT_TEST(MathSort, large_sort) {
  constexpr auto item_count = 10017;
  Int test[item_count] = {};
  for (Count i = 0; i < item_count; i++) {
    test[i] = i;
  }

  // Shuffle array
  srand(12);
  for (Count i = 0; i < item_count; i++) {
    Data::swap(test[rand() % item_count], test[rand() % item_count]);
  }

  auto sorted = sort(test);
  for (Count i = 0; i < item_count; i++) {
    EXPECT_EQ(sorted[i], i);
  }
}

PERIMORTEM_UNIT_TEST(MathSort, dynamic_types) {
  constexpr auto item_count = 37;
  Dynamic::Bytes test[item_count] = {};
  for (Count i = 0; i < item_count; i++) {
    test[i] = "test_string #"_view;
    test[i].append(Byte('0' + (i / 10)));
    test[i].append(Byte('0' + (i % 10)));
  }

  // Shuffle array
  srand(12);
  for (Count i = 0; i < item_count; i++) {
    Data::swap(test[rand() % item_count], test[rand() % item_count]);
  }

  // Sorting even on dynamic data should result in zero memory requests
  auto check_outs = Allocator::Bibliotheca::check_out_requests();
  auto sorted = sort(Access::Vector(test));
  EXPECT_EQ(check_outs, Allocator::Bibliotheca::check_out_requests());

  Dynamic::Bytes validate = {};
  for (Count i = 0; i < item_count; i++) {
    validate = "test_string #"_view;
    validate.append(Byte('0' + (i / 10)));
    validate.append(Byte('0' + (i % 10)));
    Dynamic::Bytes validate2;
    validate2 = validate;
    EXPECT_TEXT(sorted[i], validate);
  }
}