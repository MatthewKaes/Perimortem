// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/type_identity.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Validation;

class FirstIdentity {};
class SecondIdentity {};

static Harness TtxTypeIdentity = {
  .name = "Ttx::Concept::TypeIdentity"_view,
};

PERIMORTEM_UNIT_TEST(TtxTypeIdentity, stable_and_distinct) {
  U64 first = get_type_identity<FirstIdentity>();

  EXPECT_EQ(first, get_type_identity<FirstIdentity>());
  EXPECT_NOT(first == get_type_identity<SecondIdentity>());
}
