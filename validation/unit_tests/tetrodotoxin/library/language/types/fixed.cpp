// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryFixed = {
  .name = "Tetrodotoxin::Library::Language::Types::Fixed"_view,
};

PERIMORTEM_UNIT_TEST(LibraryFixed, direct_contract) {
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Types::Fixed fixed("Fixed[Unsigned_8,4]"_view, element, ::Signed_64(4));
  auto arguments = fixed.get_arguments();
  const Ttx::Model::Layouts::Ranged& layout = fixed.get_layout();

  EXPECT(fixed.is<Types::Fixed>());
  EXPECT(fixed.is<Ttx::Model::Type>());
  EXPECT(fixed.is<Abstract>());
  EXPECT_NOT(fixed.is<Generic>());
  EXPECT_TEXT(fixed.get_name(), "Fixed[Unsigned_8,4]"_view);
  EXPECT(&fixed.get_element_type() == &element);
  EXPECT_EQ(fixed.get_extent(), ::Signed_64(4));
  ASSERT_EQ(arguments.get_size(), Count(2));
  EXPECT(arguments.get_data()[0].find<const Ttx::Model::Type&>() == &element);
  EXPECT(arguments.get_data()[0].find<::Signed_64>() == nullptr);
  EXPECT(arguments.get_data()[1].find<const Ttx::Model::Type&>() == nullptr);
  ASSERT(arguments.get_data()[1].find<::Signed_64>() != nullptr);
  EXPECT_EQ(*arguments.get_data()[1].find<::Signed_64>(), ::Signed_64(4));
  EXPECT_EQ(layout.get_size(), Count(4));
  EXPECT(layout.get_abstract(0).visit(
      []() { return False; },
      [&element](const Abstract& selected) {
        return &selected == &element ? True : False;
      }));
  EXPECT(layout.get_abstract(3).visit(
      []() { return False; },
      [&element](const Abstract& selected) {
        return &selected == &element ? True : False;
      }));
  EXPECT_NOT(layout.get_abstract(4));
  EXPECT_NOT(fixed.get_documentation().is_empty());
  EXPECT(&fixed.resolve_context("member"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryFixed, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Generics::Fixed fixed_formula;
  const Generic& formula = fixed_formula;
  const Static::Vector<Generic::Argument, 2> positive = {
    {Generic::Argument(element), Generic::Argument(::Signed_64(4))},
  };
  const Static::Vector<Generic::Argument, 2> zero = {
    {Generic::Argument(element), Generic::Argument(::Signed_64(0))},
  };
  const Static::Vector<Generic::Argument, 2> negative = {
    {Generic::Argument(element), Generic::Argument(::Signed_64(-1))},
  };
  const Static::Vector<Generic::Argument, 2> wrong_element = {
    {Generic::Argument(::Unsigned_64(8)), Generic::Argument(::Signed_64(4))},
  };
  const Static::Vector<Generic::Argument, 2> wrong_extent = {
    {Generic::Argument(element), Generic::Argument(::Unsigned_64(4))},
  };
  const Static::Vector<Generic::Argument, 1> wrong_arity = {
    {Generic::Argument(element)},
  };

  auto positive_type = formula.create(positive, arena);
  ASSERT(positive_type);
  EXPECT(positive_type->is<Types::Fixed>());
  EXPECT_TEXT(positive_type->get_name(), "Fixed[Unsigned_8,4]"_view);
  EXPECT(positive_type->visit<Types::Fixed>(
      [&element](const Types::Fixed& selected) {
        return &selected.get_element_type() == &element &&
                       selected.get_extent() == ::Signed_64(4)
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));

  auto zero_type = formula.create(zero, arena);
  ASSERT(zero_type);
  EXPECT(zero_type->is<Types::Fixed>());
  EXPECT_TEXT(zero_type->get_name(), "Fixed[Unsigned_8,0]"_view);
  EXPECT(zero_type->visit<Types::Fixed>(
      [](const Types::Fixed& selected) {
        return selected.get_extent() == ::Signed_64(0) &&
                       selected.get_layout().get_size() == Count(0)
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));

  EXPECT_NOT(formula.create(negative, arena));
  EXPECT_NOT(formula.create(wrong_element, arena));
  EXPECT_NOT(formula.create(wrong_extent, arena));
  EXPECT_NOT(formula.create(wrong_arity, arena));
}
