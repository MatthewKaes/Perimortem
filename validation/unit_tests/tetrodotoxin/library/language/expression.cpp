// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/library/language/binding.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/projection.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/structured.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

class ExpressionType : public Ttx::Model::Type {
 public:
  ExpressionType(View::Bytes name, Ttx::Model::Layouts::Structured layout = {})
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Ttx::Model::Layouts::Structured& override {
    return layout;
  }

 private:
  View::Bytes name;
  Ttx::Model::Layouts::Structured layout;
};

class ExpressionField : public Ttx::Model::Addressable {
 public:
  ExpressionField(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
};

class ExpressionValue : public Expression {
 public:
  ExpressionValue(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  inline static const Ttx::Model::Layouts::Fluid inputs;
};

template <typename Catagory>
class ConstantValue : public Catagory {
 public:
  using Value = typename Catagory::Value;

  ConstantValue(View::Bytes name, const Ttx::Model::Type& type, Value value)
      : name(name), type(type), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_value() const -> Value override { return value; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  Value value;
};

static Harness LibraryExpression = {
  .name = "Tetrodotoxin::Library::Language::Expression"_view,
};

static auto selects(
    const Perimortem::Utility::Option<const Abstract&>& result,
    const Abstract& expected) -> Bool {
  return result.visit(
      []() { return False; },
      [&expected](const Abstract& selected) {
        return &selected == &expected ? True : False;
      });
}

static auto is_none(const Perimortem::Utility::Option<const Abstract&>& result)
    -> Bool {
  return result.visit(
      []() { return True; }, [](const Abstract&) { return False; });
}

PERIMORTEM_UNIT_TEST(LibraryExpression, binding_and_projection) {
  ExpressionType scalar("Scalar"_view);
  ExpressionField field("value"_view, scalar);
  ExpressionValue receiver("receiver"_view, scalar);
  Binding binding("bound"_view, receiver);
  Projection projection(receiver, field);

  EXPECT(binding.is<Expression>());
  EXPECT(binding.is<Binding>());
  EXPECT_TEXT(binding.get_name(), "bound"_view);
  EXPECT(&binding.get_type() == &scalar);
  EXPECT(&binding.get_expression() == &receiver);
  EXPECT(selects(binding.get_inputs().get_abstract(0), receiver));
  EXPECT(is_none(binding.get_inputs().get_abstract(1)));
  EXPECT(binding.fits(scalar));

  EXPECT(projection.is<Expression>());
  EXPECT(projection.is<Projection>());
  EXPECT_TEXT(projection.get_name(), "value"_view);
  EXPECT(&projection.get_type() == &scalar);
  EXPECT(&projection.get_receiver() == &receiver);
  EXPECT(&projection.get_addressable() == &field);
  EXPECT(selects(projection.get_inputs().get_abstract(0), receiver));
  EXPECT(is_none(projection.get_inputs().get_abstract(1)));
  EXPECT(projection.fits(scalar));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_identity) {
  Types::Unsigned_64 type;
  Types::Unsigned_64 other_type;
  ConstantValue<Constants::Unsigned> first(
      "first"_view, type, ::Unsigned_64(100));
  ConstantValue<Constants::Unsigned> same(
      "same"_view, type, ::Unsigned_64(100));
  ConstantValue<Constants::Unsigned> different(
      "different"_view, type, ::Unsigned_64(101));
  ConstantValue<Constants::Unsigned> other(
      "other"_view, other_type, ::Unsigned_64(100));

  EXPECT(first.is<Expression>());
  EXPECT(first.is<Constant>());
  EXPECT(first.is<Constants::Unsigned>());
  EXPECT_NOT(first.is<Ttx::Model::Type>());
  EXPECT(&first.get_type() == &type);
  EXPECT(first.get_inputs().is_empty());
  EXPECT(first == same);
  EXPECT(first != different);
  EXPECT(first != other);
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_fitting) {
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
      "fits"_view, unsigned_64, ::Unsigned_64(255));
  ConstantValue<Constants::Unsigned> needs_16(
      "wide"_view, unsigned_64, ::Unsigned_64(256));
  ConstantValue<Constants::Signed> fits_signed(
      "fits"_view, signed_64, ::Signed_64(-128));
  ConstantValue<Constants::Signed> misses_signed(
      "wide"_view, signed_64, ::Signed_64(-129));
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
