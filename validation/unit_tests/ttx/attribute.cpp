// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/attribute.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAttribute = {
  .name = "TTX::Attribute"_view,
};

PERIMORTEM_UNIT_TEST(TtxAttribute, empty) {
  Ttx::Attribute attribute;

  EXPECT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT_NOT(attribute.has_value());
  EXPECT(attribute.get_bytes().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_value) {
  Ttx::Attribute attribute("binding"_view, "uniform"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "binding"_view);
  EXPECT(attribute.get_kind() == Ttx::Attribute::Kind::Bytes);
  EXPECT_TEXT(attribute.get_bytes(), "uniform"_view);
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_only) {
  Ttx::Attribute attribute("required"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "required"_view);
  EXPECT_NOT(attribute.has_value());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, value_only) {
  Ttx::Attribute attribute(View::Bytes(), "Library"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT_TEXT(attribute.get_bytes(), "Library"_view);
}

PERIMORTEM_UNIT_TEST(TtxAttribute, scalar_values) {
  Ttx::Attribute unsigned_value("slot"_view, Bits_64(7));
  Ttx::Attribute signed_value("offset"_view, Signed_64(-2));
  Ttx::Attribute real_value("scale"_view, Real_64(0.5));
  Ttx::Attribute boolean_value("enabled"_view, True);

  EXPECT_EQ(unsigned_value.get_unsigned(), Bits_64(7));
  EXPECT_EQ(signed_value.get_signed(), Signed_64(-2));
  EXPECT_EQ(real_value.get_real(), Real_64(0.5));
  EXPECT(boolean_value.get_boolean());
  EXPECT(unsigned_value == Ttx::Attribute("slot"_view, Bits_64(7)));
  EXPECT_NOT(unsigned_value == Ttx::Attribute("slot"_view, Signed_64(7)));
}
