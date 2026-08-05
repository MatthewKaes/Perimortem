// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/access.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryAccess = {
  .name = "Tetrodotoxin::Library::Language::Types::Access"_view,
};

PERIMORTEM_UNIT_TEST(LibraryAccess, direct_contract) {
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Types::Access access("Access[Unsigned_8]"_view, element);
  auto arguments = access.get_arguments();
  const Documentation& documentation = access.get_documentation();

  EXPECT(access.is<Types::Access>());
  EXPECT(access.is<Ttx::Model::Type>());
  EXPECT(access.is<Abstract>());
  EXPECT_NOT(access.is<Generic>());
  EXPECT_TEXT(access.get_name(), "Access[Unsigned_8]"_view);
  EXPECT(&access.get_element_type() == &element);
  ASSERT_EQ(arguments.get_size(), Count(1));
  EXPECT(arguments.get_data()[0].find<const Ttx::Model::Type&>() == &element);
  ASSERT_EQ(documentation.line_count(), Count(1));
  EXPECT_TEXT(
      documentation.get_line(0),
      "Provides writable access to contiguous values."_view);
  EXPECT(&access.resolve_context("member"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryAccess, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Generics::Access access_formula;
  const Generic& formula = access_formula;
  const Static::Vector<Generic::Argument, 1> accepted = {
    {Generic::Argument(element)},
  };
  const Static::Vector<Generic::Argument, 1> wrong_category = {
    {Generic::Argument(::Unsigned_64(8))},
  };
  View::Vector<Generic::Argument> wrong_arity;

  auto created = formula.create(accepted, arena);
  ASSERT(created);
  EXPECT(created->is<Types::Access>());
  EXPECT_TEXT(created->get_name(), "Access[Unsigned_8]"_view);
  EXPECT(created->visit<Types::Access>(
      [&element](const Types::Access& selected) {
        return &selected.get_element_type() == &element ? True : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT_NOT(formula.create(wrong_category, arena));
  EXPECT_NOT(formula.create(wrong_arity, arena));
}
