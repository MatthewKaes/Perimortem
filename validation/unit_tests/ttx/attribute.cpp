// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/attribute.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAttribute = {
  .name = "Ttx::Model::Attribute"_view,
};

PERIMORTEM_UNIT_TEST(TtxAttribute, empty) {
  Ttx::Model::Attribute attribute;

  EXPECT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT_NOT(attribute.has_value());
  EXPECT(attribute.get_value().is_null());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_value) {
  Ttx::Model::Attribute attribute("binding"_view, "uniform"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "binding"_view);
  EXPECT(attribute.get_value() == "uniform"_view);
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_only) {
  Ttx::Model::Attribute attribute("required"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "required"_view);
  EXPECT_NOT(attribute.has_value());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, value_only) {
  Ttx::Model::Attribute attribute(View::Bytes(), "Library"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT(attribute.get_value() == "Library"_view);
}

PERIMORTEM_UNIT_TEST(TtxAttribute, scalar_values) {
  Ttx::Model::Attribute unsigned_value("slot"_view, Unsigned_64(7));
  Ttx::Model::Attribute signed_value("offset"_view, Signed_64(-2));
  Ttx::Model::Attribute real_value("scale"_view, Real_64(0.5));
  Ttx::Model::Attribute boolean_value("enabled"_view, True);

  EXPECT(unsigned_value.get_value() == Unsigned_64(7));
  EXPECT(signed_value.get_value() == Signed_64(-2));
  EXPECT(real_value.get_value() == Real_64(0.5));
  EXPECT(boolean_value.get_value() == True);
  EXPECT(unsigned_value == Ttx::Model::Attribute("slot"_view, Unsigned_64(7)));
  EXPECT_NOT(
      unsigned_value == Ttx::Model::Attribute("slot"_view, Signed_64(7)));
}
