// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/binding.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
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
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
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
      : Expression({}), name(name), type(type) {}

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

static_assert(!__is_constructible(ExpressionValue, const ExpressionValue&));
static_assert(
    !__is_constructible(Constants::Unsigned, const Constants::Unsigned&));

class ExpressionContext : public Abstract {
 public:
  constexpr ExpressionContext(const Ttx::Model::Addressable& selected)
      : selected(selected) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "ExpressionContext"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == selected.get_name()) {
      return selected;
    }

    return Invalid::get_invalid();
  }

 private:
  const Ttx::Model::Addressable& selected;
};

class ExpressionMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  ExpressionMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "ExpressionMonograph"_view;
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
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
  Allocator::Arena arena;
  ExpressionType scalar("Scalar"_view);
  ExpressionField field("value"_view, scalar);
  ExpressionValue receiver("receiver"_view, scalar);
  auto& binding = Binding::create_synthetic(arena, "bound"_view, receiver);
  auto& projection = Projection::create_synthetic(arena, receiver, field);

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

PERIMORTEM_UNIT_TEST(LibraryExpression, wrappers_link_retained_inputs) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ExpressionType scalar("Scalar"_view);
  ExpressionField field("value"_view, scalar);
  ExpressionContext context(field);
  ExpressionMonograph graph(arena);
  Token binding_token(0, 1, 1, 5, Code::Type::Addressable);
  Token projection_token(6, 1, 7, 5, Code::Type::Addressable);
  auto binding_anchor = Anchor::create(binding_token, Span(binding_token));
  auto projection_anchor =
      Anchor::create(projection_token, Span(projection_token));
  auto& binding_input =
      Identifier::create_authored(arena, "value"_view, binding_anchor);
  auto& projection_input =
      Identifier::create_authored(arena, "value"_view, projection_anchor);
  auto& binding = Binding::create_synthetic(arena, "bound"_view, binding_input);
  auto& projection =
      Projection::create_synthetic(arena, projection_input, field);

  EXPECT_NOT(binding_input.get_addressable());
  EXPECT_NOT(projection_input.get_addressable());
  ASSERT(binding.link(graph, context, materializations));
  ASSERT(projection.link(graph, context, materializations));
  EXPECT(&*binding_input.get_addressable() == &field);
  EXPECT(&*projection_input.get_addressable() == &field);
  EXPECT(graph.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_identity) {
  Allocator::Arena arena;
  Types::Unsigned_64 type;
  Types::Unsigned_64 other_type;
  Types::Signed_64 signed_type;
  auto& first =
      Constants::Unsigned::create_synthetic(arena, type, ::Unsigned_64(100));
  auto& same =
      Constants::Unsigned::create_synthetic(arena, type, ::Unsigned_64(100));
  auto& different =
      Constants::Unsigned::create_synthetic(arena, type, ::Unsigned_64(101));
  auto& other = Constants::Unsigned::create_synthetic(
      arena, other_type, ::Unsigned_64(100));
  auto& signed_value =
      Constants::Signed::create_synthetic(arena, signed_type, ::Signed_64(100));

  EXPECT(first.is<Expression>());
  EXPECT(first.is<Constant>());
  EXPECT(first.is<Constants::Unsigned>());
  EXPECT_NOT(first.is<Ttx::Model::Type>());
  EXPECT(&first.get_type() == &type);
  EXPECT_TEXT(first.get_name(), type.get_name());
  EXPECT(first.get_value() == 100);
  EXPECT(first.get_inputs().is_empty());
  EXPECT(first == same);
  EXPECT(first != different);
  EXPECT(first != other);
  EXPECT(first != signed_value);

  auto first_fold = first.fold();
  auto repeated_fold = first.fold();
  EXPECT(first_fold.visit(
      [&](const Perimortem::Utility::Option<Expression&>& selected) {
        return selected && &*selected == &first ? True : False;
      },
      [](const Expression::Error&) { return False; }));
  EXPECT(repeated_fold.visit(
      [&](const Perimortem::Utility::Option<Expression&>& selected) {
        return selected && &*selected == &first ? True : False;
      },
      [](const Expression::Error&) { return False; }));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_fitting) {
  Allocator::Arena arena;
  Types::Unsigned_64 unsigned_64;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Signed_64 signed_64;
  Types::Signed_8 signed_8;
  Types::Boolean source_flag;
  Types::Boolean target_flag;
  Types::Real_64 real_64;
  Types::Real_64 other_real_64;
  auto& fits_8 = Constants::Unsigned::create_synthetic(
      arena, unsigned_64, ::Unsigned_64(255));
  auto& needs_16 = Constants::Unsigned::create_synthetic(
      arena, unsigned_64, ::Unsigned_64(256));
  auto& fits_signed =
      Constants::Signed::create_synthetic(arena, signed_64, ::Signed_64(-128));
  auto& misses_signed =
      Constants::Signed::create_synthetic(arena, signed_64, ::Signed_64(-129));
  auto& true_flag = Constants::True::create_synthetic(arena, source_flag);
  auto& false_flag = Constants::False::create_synthetic(arena, source_flag);
  auto& real = Constants::Real::create_synthetic(arena, real_64, Real_64(0.5));

  EXPECT(fits_8.fits(unsigned_8));
  EXPECT_NOT(needs_16.fits(unsigned_8));
  EXPECT(needs_16.fits(unsigned_16));
  EXPECT(fits_signed.fits(signed_8));
  EXPECT_NOT(misses_signed.fits(signed_8));
  EXPECT(true_flag.is<Constants::Flag>());
  EXPECT(true_flag.is<Constants::True>());
  EXPECT_NOT(true_flag.is<Constants::False>());
  EXPECT(false_flag.is<Constants::Flag>());
  EXPECT(false_flag.is<Constants::False>());
  EXPECT_NOT(false_flag.is<Constants::True>());
  EXPECT(true_flag.get_value());
  EXPECT_NOT(false_flag.get_value());
  EXPECT(true_flag.fits(target_flag));
  EXPECT(false_flag.fits(target_flag));
  EXPECT(true_flag != false_flag);
  EXPECT(real.fits(real_64));
  EXPECT_NOT(real.fits(other_real_64));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, byte_lifetime) {
  Allocator::Arena arena;
  ExpressionType type("Bytes"_view);
  ExpressionType other_type("Other Bytes"_view);
  Constants::Bytes* retained = nullptr;

  {
    Dynamic::Bytes source("stable bytes"_view);
    View::Bytes stable = arena.proxy(source);
    retained = &Constants::Bytes::create_synthetic(arena, type, stable);
  }

  auto& same =
      Constants::Bytes::create_synthetic(arena, type, "stable bytes"_view);
  auto& other = Constants::Bytes::create_synthetic(
      arena, other_type, "stable bytes"_view);
  auto& empty = Constants::Bytes::create_synthetic(arena, type, {});
  auto& also_empty = Constants::Bytes::create_synthetic(arena, type, {});

  EXPECT_TEXT(retained->get_name(), type.get_name());
  EXPECT_TEXT(retained->get_value(), "stable bytes"_view);
  EXPECT(*retained == same);
  EXPECT(*retained != other);
  EXPECT(empty.get_value().is_empty());
  EXPECT(empty == also_empty);
}
