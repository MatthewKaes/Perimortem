// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/access/vector.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreAccessBytes = {
  .name = "Core::Access::Bytes"_view,
};

static Harness CoreAccessVector = {
  .name = "Core::Access::Vector"_view,
};

PERIMORTEM_UNIT_TEST(CoreAccessBytes, borrowed_element) {
  Unsigned_8 storage[] = {'a', 'b'};
  Access::Bytes bytes(storage);

  auto first = bytes[0];
  auto last = bytes.at(1);

  EXPECT(first);
  EXPECT(last);
  EXPECT_EQ(&*first, &storage[0]);
  EXPECT_EQ(&*last, &storage[1]);

  *first = 'c';
  EXPECT_EQ(storage[0], Unsigned_8('c'));
}

PERIMORTEM_UNIT_TEST(CoreAccessBytes, unavailable_element) {
  Unsigned_8 storage[] = {'a', 'b'};
  Access::Bytes bytes(storage);

  EXPECT_NOT(bytes[2]);
  EXPECT_NOT(bytes.at(Count(-1)));
  EXPECT_NOT(Access::Bytes()[0]);
}

PERIMORTEM_UNIT_TEST(CoreAccessVector, borrowed_element) {
  Unsigned_32 storage[] = {7, 9};
  Access::Vector<Unsigned_32> values(storage);

  auto first = values[0];
  auto last = values.at(1);

  EXPECT(first);
  EXPECT(last);
  EXPECT_EQ(&*first, &storage[0]);
  EXPECT_EQ(&*last, &storage[1]);

  *last = 11;
  EXPECT_EQ(storage[1], Unsigned_32(11));
}

PERIMORTEM_UNIT_TEST(CoreAccessVector, unavailable_element) {
  Unsigned_32 storage[] = {7, 9};
  Access::Vector<Unsigned_32> values(storage);

  EXPECT_NOT(values[2]);
  EXPECT_NOT(values.at(Count(-1)));
  EXPECT_NOT(Access::Vector<Unsigned_32>()[0]);
}
