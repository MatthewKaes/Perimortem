// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryRange = {
  .name = "Tetrodotoxin::Library::Language::Operations::Range"_view,
};

class RangeMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  RangeMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  auto get_name() const -> View::Bytes override {
    return "RangeMonograph"_view;
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class RangeExpression : public Expression {
 public:
  RangeExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto get_type() const -> const Abstract& override { return type; }

  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Abstract& type;
  Ttx::Model::Layouts::Fluid inputs;
};

static auto input_is(
    const Operations::Range& range,
    Count index,
    const Expression& expected) -> Bool {
  return range.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

static auto fold_is_dynamic(Operations::Range& range) -> Bool {
  return range.fold().visit(
      [](const Option<Expression&>& selected) { return Bool(!selected); },
      [](const Expression::Error&) { return False; });
}

PERIMORTEM_UNIT_TEST(LibraryRange, exact_materialization) {
  Allocator::Arena domain;
  RangeMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Signed_8 signed_8;
  RangeExpression unsigned_start("unsigned start"_view, unsigned_8);
  RangeExpression unsigned_end("unsigned end"_view, unsigned_8);
  RangeExpression signed_start("signed start"_view, signed_8);
  RangeExpression signed_end("signed end"_view, signed_8);
  auto& first = Operations::Range::create_synthetic(
      domain, materializations, unsigned_start, unsigned_end);
  auto& repeated = Operations::Range::create_synthetic(
      domain, materializations, unsigned_start, unsigned_end);
  auto& signed_range = Operations::Range::create_synthetic(
      domain, materializations, signed_start, signed_end);

  ASSERT(first.is<Operations::Range>());
  EXPECT(first.implements(Operations::Range::contract_id));
  EXPECT(input_is(first, 0, unsigned_start));
  EXPECT(input_is(first, 1, unsigned_end));
  ASSERT(first.link(source, Invalid::get_invalid(), materializations));
  ASSERT(first.link(source, Invalid::get_invalid(), materializations));
  ASSERT(repeated.link(source, Invalid::get_invalid(), materializations));
  ASSERT(signed_range.link(source, Invalid::get_invalid(), materializations));
  ASSERT(first.get_type().is<Types::Range>());
  ASSERT(signed_range.get_type().is<Types::Range>());
  const auto& first_type = static_cast<const Types::Range&>(first.get_type());
  const auto& signed_type =
      static_cast<const Types::Range&>(signed_range.get_type());
  EXPECT(&first.get_type() == &repeated.get_type());
  EXPECT(&first_type.get_element_type() == &unsigned_8);
  EXPECT(&signed_type.get_element_type() == &signed_8);
  EXPECT_EQ(materializations.get_size(), Count(2));
  EXPECT(fold_is_dynamic(first));
  EXPECT(source.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryRange, integer_legality) {
  Allocator::Arena domain;
  RangeMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_32 real_32;
  RangeExpression unsigned_value("unsigned"_view, unsigned_8);
  RangeExpression other_width("other width"_view, unsigned_16);
  RangeExpression flag("flag"_view, Tetrodotoxin::Library::Dialect::get_bool());
  RangeExpression real("real"_view, real_32);
  RangeExpression unresolved("unresolved"_view, Invalid::get_invalid());
  auto& mismatch = Operations::Range::create_synthetic(
      domain, materializations, unsigned_value, other_width);
  auto& bool_range =
      Operations::Range::create_synthetic(domain, materializations, flag, flag);
  auto& real_range =
      Operations::Range::create_synthetic(domain, materializations, real, real);
  auto& unresolved_range = Operations::Range::create_synthetic(
      domain, materializations, unsigned_value, unresolved);

  EXPECT_NOT(mismatch.link(source, Invalid::get_invalid(), materializations));
  EXPECT_NOT(bool_range.link(source, Invalid::get_invalid(), materializations));
  EXPECT_NOT(real_range.link(source, Invalid::get_invalid(), materializations));
  EXPECT_NOT(
      unresolved_range.link(source, Invalid::get_invalid(), materializations));
  EXPECT_EQ(source.get_diagnostics().get_size(), Count(4));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(LibraryRange, materializations_owner) {
  Allocator::Arena domain;
  RangeMonograph source(domain);
  Materializations retained(domain);
  Materializations other(domain);
  Types::Unsigned_8 unsigned_8;
  RangeExpression start("start"_view, unsigned_8);
  RangeExpression end("end"_view, unsigned_8);
  auto& range =
      Operations::Range::create_synthetic(domain, retained, start, end);

  EXPECT_NOT(range.link(source, Invalid::get_invalid(), other));
  EXPECT_EQ(source.get_diagnostics().get_size(), Count(1));
  EXPECT_EQ(retained.get_size(), Count(0));
  EXPECT_EQ(other.get_size(), Count(0));
}
