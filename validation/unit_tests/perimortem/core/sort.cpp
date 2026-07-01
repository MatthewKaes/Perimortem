// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/algorithm/sort.hpp"

#include "validation/unit_test.hpp"

#include <stdlib.h>

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

using namespace Validation;

namespace Perimortem::Memory::Dynamic {
auto operator>(const Bytes& lhs, const Bytes& rhs) -> Bool {
  if (lhs.get_size() != rhs.get_size()) {
    return lhs.get_size() > rhs.get_size();
  }

  View::Bytes data = lhs;
  View::Bytes comparison_bytes = rhs;
  for (Count i = 0; i < data.get_size(); i++) {
    if (data[i] != comparison_bytes.get_data()[i]) {
      return data[i] > comparison_bytes.get_data()[i];
    }
  }

  return false;
}
}  // namespace Perimortem::Memory::Dynamic

static Harness AlgoSort = {
  .name = "Core::Algorithm::Sort"_view,
};

PERIMORTEM_UNIT_TEST(AlgoSort, simple_sort) {
  constexpr Signed_32 test_const[] = {9, 8, 1, 4, 3, 7};
  Signed_32 test[] = {9, 8, 1, 4, 3, 7};

  constexpr auto sorted_const = Algorithm::sort(test_const);
  auto sorted = Algorithm::sort(Access::Vector(test));

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

PERIMORTEM_UNIT_TEST(AlgoSort, empty_sort) {
  constexpr auto sorted_const = Algorithm::sort(Access::Vector<Signed_32>());
  auto sorted = Algorithm::sort(Access::Vector<Signed_32>());

  EXPECT_EQ(sorted.get_size(), 0);
  EXPECT_EQ(sorted_const.get_size(), 0);
}

PERIMORTEM_UNIT_TEST(AlgoSort, large_sort) {
  constexpr auto item_count = 10017;
  Signed_32 test[item_count] = {};
  for (Count i = 0; i < item_count; i++) {
    test[i] = i;
  }

  // Shuffle array
  srand(12);
  for (Count i = 0; i < item_count; i++) {
    Data::swap(test[rand() % item_count], test[rand() % item_count]);
  }

  auto sorted = Algorithm::sort(test);
  for (Count i = 0; i < item_count; i++) {
    EXPECT_EQ(sorted[i], i);
  }
}

PERIMORTEM_UNIT_TEST(AlgoSort, dynamic_types) {
  constexpr auto item_count = 37;
  Dynamic::Bytes test[item_count] = {};
  for (Count i = 0; i < item_count; i++) {
    test[i] = "test_string #"_view;
    test[i].append(Bits_8('0' + (i / 10)));
    test[i].append(Bits_8('0' + (i % 10)));
  }

  // Shuffle array
  srand(12);
  for (Count i = 0; i < item_count; i++) {
    Data::swap(test[rand() % item_count], test[rand() % item_count]);
  }

  // Sorting even on dynamic data should result in zero memory requests
  auto check_outs = Bibliotheca::check_out_requests();
  auto sorted = Algorithm::sort(Access::Vector(test));
  EXPECT_EQ(check_outs, Bibliotheca::check_out_requests());

  Dynamic::Bytes validate = {};
  for (Count i = 0; i < item_count; i++) {
    validate = "test_string #"_view;
    validate.append(Bits_8('0' + (i / 10)));
    validate.append(Bits_8('0' + (i % 10)));
    EXPECT_TEXT(sorted[i].get_view(), validate.get_view());
  }
}
