// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/range.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryRange = {
  .name = "Tetrodotoxin::Library::Language::Types::Range"_view,
};

PERIMORTEM_UNIT_TEST(LibraryRange, direct_contract) {
  Types::Range range("Range[Signed_16]"_view, Dialect::get_signed_16());
  auto arguments = range.get_arguments();

  EXPECT(range.is<Types::Range>());
  EXPECT(range.is<Ttx::Model::Type>());
  EXPECT(range.is<Abstract>());
  EXPECT_NOT(range.is<Generic>());
  EXPECT_TEXT(range.get_name(), "Range[Signed_16]"_view);
  EXPECT(&range.get_element_type() == &Dialect::get_signed_16());
  ASSERT_EQ(arguments.get_size(), Count(1));
  EXPECT(
      arguments.get_data()[0].find<const Ttx::Model::Type&>() ==
      &Dialect::get_signed_16());
  ASSERT_EQ(range.get_layout().get_size(), Count(1));
  EXPECT(&*range.get_layout().get_abstract(0) == &range);
  EXPECT_NOT(range.get_documentation().is_empty());
  EXPECT(&range.resolve_context("member"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryRange, formula_legality) {
  Allocator::Arena domain;
  const Generic& formula = Generics::Range::get_formula();
  const Static::Vector<Generic::Argument, 1> signed_argument = {{
    Generic::Argument(Dialect::get_signed_8()),
  }};
  const Static::Vector<Generic::Argument, 1> unsigned_argument = {{
    Generic::Argument(Dialect::get_unsigned_64()),
  }};
  const Static::Vector<Generic::Argument, 1> bool_argument = {{
    Generic::Argument(Dialect::get_bool()),
  }};
  const Static::Vector<Generic::Argument, 1> real_argument = {{
    Generic::Argument(Dialect::get_real_32()),
  }};
  const Static::Vector<Generic::Argument, 1> wrong_kind = {{
    Generic::Argument(Unsigned_64(1)),
  }};
  View::Vector<Generic::Argument> wrong_arity;

  auto signed_type = formula.create(signed_argument, domain);
  auto unsigned_type = formula.create(unsigned_argument, domain);
  ASSERT(signed_type && unsigned_type);
  EXPECT(signed_type->is<Types::Range>());
  EXPECT(unsigned_type->is<Types::Range>());
  EXPECT(signed_type->visit<Types::Range>(
      [](const Types::Range& selected) {
        return &selected.get_element_type() == &Dialect::get_signed_8() ? True
                                                                        : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT(unsigned_type->visit<Types::Range>(
      [](const Types::Range& selected) {
        return &selected.get_element_type() == &Dialect::get_unsigned_64()
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT_NOT(formula.create(bool_argument, domain));
  EXPECT_NOT(formula.create(real_argument, domain));
  EXPECT_NOT(formula.create(wrong_kind, domain));
  EXPECT_NOT(formula.create(wrong_arity, domain));
}

PERIMORTEM_UNIT_TEST(LibraryRange, materialization_identity) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  const Static::Vector<Generic::Argument, 1> first_argument = {{
    Generic::Argument(Dialect::get_unsigned_8()),
  }};
  const Static::Vector<Generic::Argument, 1> second_argument = {{
    Generic::Argument(Dialect::get_unsigned_16()),
  }};

  auto first = materializations.materialize(
      Generics::Range::get_formula(), first_argument);
  auto repeated = materializations.materialize(
      Generics::Range::get_formula(), first_argument);
  auto second = materializations.materialize(
      Generics::Range::get_formula(), second_argument);

  ASSERT(first && repeated && second);
  EXPECT(&*first == &*repeated);
  EXPECT(&*first != &*second);
  EXPECT_TEXT(first->get_name(), "Range[Unsigned_8]"_view);
  EXPECT_TEXT(second->get_name(), "Range[Unsigned_16]"_view);
  EXPECT_EQ(materializations.get_size(), Count(2));
}
