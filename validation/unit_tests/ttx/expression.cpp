// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/abstraction/alias.hpp"
#include "ttx/abstraction/invalid.hpp"
#include "ttx/model/argument.hpp"
#include "ttx/model/constants/bytes.hpp"
#include "ttx/model/constants/flag.hpp"
#include "ttx/model/constants/real.hpp"
#include "ttx/model/constants/signed.hpp"
#include "ttx/model/constants/unsigned.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/types/real.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A terminal fixture keeps target width facts on the Type selected by the
/// toolchain. Constant values can then prove contextual fitting independently.
template <typename Contract>
class ConstantTerminal final : public Contract {
 public:
  ConstantTerminal(
      View::Bytes name,
      Count size,
      Count alignment,
      const Invalid& invalid)
      : name(name), size(size), alignment(alignment), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_size() const -> Count override { return size; }
  auto get_alignment() const -> Count override { return alignment; }

 private:
  View::Bytes name;
  Count size;
  Count alignment;
  const Invalid& invalid;
};

/// Byte arrays may use an ordinary composite Type. Their Constant contract
/// does not pretend that bytes are a language-level String or one terminal.
class BytesType final : public Type {
 public:
  BytesType(View::Bytes name, const Invalid& invalid)
      : name(name), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  const Invalid& invalid;
  Structured layout;
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

/// Target layout slots remain Addressables. Their resolution supplies the Type
/// against which a source Expression proves that it fits.
class ConstantField final : public Addressable {
 public:
  ConstantField(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Type& type;
};

static Harness TtxExpression = {
  .name = "TTX::Expression"_view,
};

PERIMORTEM_UNIT_TEST(TtxExpression, constant_identity) {
  Invalid invalid;
  ConstantTerminal<Types::Unsigned> unsigned_64(
      "Unsigned_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Unsigned> other_unsigned_64(
      "OtherUnsigned_64"_view, 8, 8, invalid);
  ConstantValue<Constants::Unsigned> first(
      "first"_view, unsigned_64, Bits_64(100));
  ConstantValue<Constants::Unsigned> same(
      "same"_view, unsigned_64, Bits_64(100));
  ConstantValue<Constants::Unsigned> different(
      "different"_view, unsigned_64, Bits_64(101));
  ConstantValue<Constants::Unsigned> other_type(
      "other"_view, other_unsigned_64, Bits_64(100));

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
  Invalid invalid;
  ConstantTerminal<Types::Signed> signed_type("Signed_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Real> real_type("Real_128"_view, 16, 16, invalid);
  ConstantTerminal<Types::Flag> flag_type("Flag"_view, 1, 1, invalid);
  BytesType bytes_type("Bytes"_view, invalid);
  ConstantValue<Constants::Signed> signed_value(
      "negative"_view, signed_type, Signed_64(-20));
  ConstantValue<Constants::Real> real_value(
      "fraction"_view, real_type, Real_128(0.5));
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
  EXPECT(real_value.get_value() == Real_128(0.5));
  EXPECT(first_nan == same_nan);
  EXPECT(flag_value.get_value());
  EXPECT_TEXT(first_bytes.get_value(), "data"_view);
  EXPECT(first_bytes == same_bytes);
  EXPECT(first_bytes != other_bytes);
  EXPECT_NOT(first_bytes.is<Constants::Unsigned>());
}

PERIMORTEM_UNIT_TEST(TtxExpression, constant_fitting) {
  Invalid invalid;
  ConstantTerminal<Types::Unsigned> unsigned_64(
      "Unsigned_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Unsigned> unsigned_8(
      "Unsigned_8"_view, 1, 1, invalid);
  ConstantTerminal<Types::Unsigned> unsigned_16(
      "Unsigned_16"_view, 2, 2, invalid);
  ConstantTerminal<Types::Signed> signed_64("Signed_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Signed> signed_8("Signed_8"_view, 1, 1, invalid);
  ConstantTerminal<Types::Flag> source_flag("FlagA"_view, 1, 1, invalid);
  ConstantTerminal<Types::Flag> target_flag("FlagB"_view, 4, 4, invalid);
  ConstantTerminal<Types::Real> real_64("Real_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Real> other_real_64(
      "OtherReal_64"_view, 8, 8, invalid);
  ConstantValue<Constants::Unsigned> fits_8(
      "fits"_view, unsigned_64, Bits_64(255));
  ConstantValue<Constants::Unsigned> needs_16(
      "wide"_view, unsigned_64, Bits_64(256));
  ConstantValue<Constants::Signed> fits_signed(
      "fits"_view, signed_64, Signed_64(-128));
  ConstantValue<Constants::Signed> misses_signed(
      "wide"_view, signed_64, Signed_64(-129));
  ConstantValue<Constants::Flag> flag("flag"_view, source_flag, True);
  ConstantValue<Constants::Real> real("real"_view, real_64, Real_128(0.5));

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
  Invalid invalid;
  ConstantTerminal<Types::Unsigned> unsigned_64(
      "Unsigned_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Unsigned> unsigned_8(
      "Unsigned_8"_view, 1, 1, invalid);
  ConstantTerminal<Types::Signed> signed_64("Signed_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Signed> signed_8("Signed_8"_view, 1, 1, invalid);
  ConstantValue<Constants::Unsigned> x("x"_view, unsigned_64, Bits_64(255));
  ConstantValue<Constants::Signed> y("y"_view, signed_64, Signed_64(-20));
  ConstantField x_field("x"_view, unsigned_8);
  ConstantField y_field("y"_view, signed_8);
  const Reference<Abstract> ordered[] = {x, y};
  const Reference<Abstract> reordered[] = {y, x};
  const Reference<Addressable> fields[] = {x_field, y_field};
  Fluid fluid(ordered);
  Named named(reordered);
  Structured target(fields);

  EXPECT(fluid.fits(target));
  EXPECT(named.fits(target));
  EXPECT(&fluid.get_fitted(target, 0) == &x);
  EXPECT(&named.get_fitted(target, 0) == &x);
  EXPECT(&named.get_fitted(target, 1) == &y);
}

PERIMORTEM_UNIT_TEST(TtxExpression, argument_values) {
  Invalid invalid;
  ConstantTerminal<Types::Unsigned> unsigned_64(
      "Unsigned_64"_view, 8, 8, invalid);
  ConstantTerminal<Types::Unsigned> other_unsigned_64(
      "OtherUnsigned_64"_view, 8, 8, invalid);
  ConstantValue<Constants::Unsigned> first(
      "first"_view, unsigned_64, Bits_64(100));
  ConstantValue<Constants::Unsigned> same(
      "same"_view, unsigned_64, Bits_64(100));
  ConstantValue<Constants::Unsigned> different(
      "different"_view, unsigned_64, Bits_64(101));
  ConstantValue<Constants::Unsigned> other_type(
      "other"_view, other_unsigned_64, Bits_64(100));
  BytesType bytes_type("Bytes"_view, invalid);
  ConstantValue<Constants::Bytes> first_bytes(
      "first_bytes"_view, bytes_type, "cache"_view);
  ConstantValue<Constants::Bytes> same_bytes(
      "same_bytes"_view, bytes_type, "cache"_view);
  Alias alias("Hundred"_view, first);

  EXPECT(Argument(first) == Argument(same));
  EXPECT(Argument(alias) == Argument(first));
  EXPECT(Argument(first) != Argument(different));
  EXPECT(Argument(first) != Argument(other_type));
  EXPECT(Argument(first_bytes) == Argument(same_bytes));
  EXPECT(Argument(Bits_64(100)) == Argument(Bits_64(100)));
  EXPECT(Argument(Bits_64(100)) != Argument(Bits_64(101)));
}
