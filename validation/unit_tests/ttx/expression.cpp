// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/constants/bytes.hpp"
#include "ttx/model/constants/flag.hpp"
#include "ttx/model/constants/real.hpp"
#include "ttx/model/constants/signed.hpp"
#include "ttx/model/constants/unsigned.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/types/bool.hpp"
#include "ttx/model/types/real_64.hpp"
#include "ttx/model/types/signed_64.hpp"
#include "ttx/model/types/signed_8.hpp"
#include "ttx/model/types/unsigned_16.hpp"
#include "ttx/model/types/unsigned_64.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// Byte arrays may use an ordinary composite Type. Their Constant contract
/// does not pretend that bytes are a language-level String or one terminal.
class ByteArrayType final : public Type {
 public:
  ByteArrayType(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  inline static const Structured layout;
};

/// Constant implementations publish one immutable payload and borrow the Type
/// that gives that payload semantic meaning. The shared contract owns fitting.
template <typename Contract>
class ConstantValue final : public Contract {
 public:
  using Value = typename Contract::Value;

  ConstantValue(View::Bytes name, const Type& type, Value value)
      : name(name), type(type), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_value() const -> Value override { return value; }

 private:
  View::Bytes name;
  const Type& type;
  Value value;
};

/// Target layout slots remain Addressables. Their type query supplies the Type
/// against which a source Expression proves that it fits.
class ConstantField final : public Addressable {
 public:
  ConstantField(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

static Harness TtxExpression = {
  .name = "TTX::Expression"_view,
};

PERIMORTEM_UNIT_TEST(TtxExpression, constant_identity) {
  Types::Unsigned_64 unsigned_64;
  Types::Unsigned_64 other_unsigned_64;
  ConstantValue<Constants::Unsigned> first(
      "first"_view, unsigned_64, Unsigned_64(100));
  ConstantValue<Constants::Unsigned> same(
      "same"_view, unsigned_64, Unsigned_64(100));
  ConstantValue<Constants::Unsigned> different(
      "different"_view, unsigned_64, Unsigned_64(101));
  ConstantValue<Constants::Unsigned> other_type(
      "other"_view, other_unsigned_64, Unsigned_64(100));

  EXPECT(first.is<Expression>());
  EXPECT(first.is<Constant>());
  EXPECT(first.is<Constants::Unsigned>());
  EXPECT_NOT(first.is<Type>());
  EXPECT(&first.resolve() == &first);
  EXPECT(&first.get_type() == &unsigned_64);
  EXPECT(first.resolve_context("Anything"_view).is<Invalid>());
  EXPECT(first.get_inputs().is_empty());
  EXPECT(first == same);
  EXPECT(first != different);
  EXPECT(first != other_type);
}

PERIMORTEM_UNIT_TEST(TtxExpression, constant_domains) {
  Types::Signed_64 signed_type;
  Types::Real_64 real_type;
  Types::Boolean flag_type;
  ByteArrayType bytes_type("ByteArray"_view);
  ConstantValue<Constants::Signed> signed_value(
      "negative"_view, signed_type, Signed_64(-20));
  ConstantValue<Constants::Real> real_value(
      "fraction"_view, real_type, Real_64(0.5));
  ConstantValue<Constants::Real> first_nan(
      "first_nan"_view, real_type, __builtin_nanl(""));
  ConstantValue<Constants::Real> same_nan(
      "same_nan"_view, real_type, __builtin_nanl(""));
  ConstantValue<Constants::Flag> flag_value("enabled"_view, flag_type, True);
  ConstantValue<Constants::Bytes> first_bytes(
      "first"_view, bytes_type, "data"_view);
  ConstantValue<Constants::Bytes> same_bytes(
      "same"_view, bytes_type, "data"_view);
  ConstantValue<Constants::Bytes> other_bytes(
      "other"_view, bytes_type, "date"_view);

  EXPECT_EQ(signed_value.get_value(), Signed_64(-20));
  EXPECT(real_value.get_value() == Real_64(0.5));
  EXPECT(first_nan == same_nan);
  EXPECT(flag_value.get_value());
  EXPECT_TEXT(first_bytes.get_value(), "data"_view);
  EXPECT(first_bytes == same_bytes);
  EXPECT(first_bytes != other_bytes);
  EXPECT_NOT(first_bytes.is<Constants::Unsigned>());
}

PERIMORTEM_UNIT_TEST(TtxExpression, constant_fitting) {
  Types::Unsigned_64 unsigned_64;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Signed_64 signed_64;
  Types::Signed_8 signed_8;
  Types::Boolean source_flag;
  Types::Boolean target_flag;
  Types::Real_64 real_64;
  Types::Real_64 other_real_64;
  ConstantValue<Constants::Unsigned> fits_8(
      "fits"_view, unsigned_64, Unsigned_64(255));
  ConstantValue<Constants::Unsigned> needs_16(
      "wide"_view, unsigned_64, Unsigned_64(256));
  ConstantValue<Constants::Signed> fits_signed(
      "fits"_view, signed_64, Signed_64(-128));
  ConstantValue<Constants::Signed> misses_signed(
      "wide"_view, signed_64, Signed_64(-129));
  ConstantValue<Constants::Flag> flag("flag"_view, source_flag, True);
  ConstantValue<Constants::Real> real("real"_view, real_64, Real_64(0.5));

  EXPECT(fits_8.fits(unsigned_8));
  EXPECT_NOT(needs_16.fits(unsigned_8));
  EXPECT(needs_16.fits(unsigned_16));
  EXPECT(fits_signed.fits(signed_8));
  EXPECT_NOT(misses_signed.fits(signed_8));
  EXPECT(flag.fits(target_flag));
  EXPECT(real.fits(real_64));
  EXPECT_NOT(real.fits(other_real_64));
}

PERIMORTEM_UNIT_TEST(TtxExpression, layout_constants) {
  Types::Unsigned_64 unsigned_64;
  Types::Unsigned_8 unsigned_8;
  Types::Signed_64 signed_64;
  Types::Signed_8 signed_8;
  ConstantValue<Constants::Unsigned> x("x"_view, unsigned_64, Unsigned_64(255));
  ConstantValue<Constants::Signed> y("y"_view, signed_64, Signed_64(-20));
  ConstantField x_field("x"_view, unsigned_8);
  ConstantField y_field("y"_view, signed_8);
  const Static::Vector<Reference<Abstract>, 2> ordered = {{x, y}};
  const Static::Vector<Reference<Abstract>, 2> reordered = {{y, x}};
  const Static::Vector<Reference<Addressable>, 2> fields = {{x_field, y_field}};
  Fluid fluid(ordered);
  Named named(reordered);
  Structured target(fields);

  EXPECT(fluid.fits(target));
  EXPECT(named.fits(target));
  EXPECT(&fluid.get_fitted(target, 0) == &x);
  EXPECT(&named.get_fitted(target, 0) == &x);
  EXPECT(&named.get_fitted(target, 1) == &y);
}
