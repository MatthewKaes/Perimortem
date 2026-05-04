// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/math/sort.hpp"
#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::System;

using namespace Validation;

Test::Harness SystemRandom = {.name = "System::Random"};

PERIMORTEM_UNIT_TEST(SystemRandom, entropy_check) {
  Static::Vector<Bits_64, 8> values;

  for (Count i = 0; i < values.get_size(); i++) {
    values[i] = Random::read_entropy();
  }

  sort(values.get_access());

  Count collisions = 0;
  Count zero_values = 0;
  for (Count i = 1; i < values.get_size(); i++) {
    zero_values += values[i] == 0;
    collisions += values[i - 1] == values[i];
  }
  zero_values += values[0] == 0;

  // Ensure that entropy for the system can at least provide 8 reasonable
  // values for the Philox generator.
  EXPECT_EQ(collisions, 0);

  // Getting ever more than one zero in 8 entropy reads implies something is
  // wrong with the entropy source.
  EXPECT(zero_values < 2);
}

PERIMORTEM_UNIT_TEST(SystemRandom, stress_test) {
  Static::Vector<Bits_64, 100'000> values;

  for (Count i = 0; i < values.get_size(); i++) {
    values[i] = Random::generate();
  }

  sort(values.get_access());

  Count collisions = 0;
  for (Count i = 1; i < values.get_size(); i++) {
    collisions += values[i - 1] == values[i];
  }

  // Ensure the Philox generation is working correctly.
  EXPECT_EQ(collisions, 0);
}
