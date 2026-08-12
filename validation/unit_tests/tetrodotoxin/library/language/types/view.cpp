// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/view.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryView = {
  .name = "Tetrodotoxin::Library::Language::Types::View"_view,
};

PERIMORTEM_UNIT_TEST(LibraryView, direct_contract) {
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Types::View view("View[Unsigned_8]"_view, element);

  EXPECT(view.is<Types::View>());
  EXPECT(view.is<Ttx::Model::Type>());
  EXPECT(view.is<Abstract>());
  EXPECT_NOT(view.is<Generic>());
  EXPECT_TEXT(view.get_name(), "View[Unsigned_8]"_view);
  EXPECT(&view.get_element_type() == &element);
  EXPECT_NOT(view.get_documentation().is_empty());
  EXPECT(&view.resolve_context("member"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryView, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Generics::View view_formula;
  const Generic& formula = view_formula;
  const Static::Vector<Generic::Argument, 1> accepted = {
    {Generic::Argument(element)},
  };
  const Static::Vector<Generic::Argument, 1> wrong_category = {
    {Generic::Argument(::Unsigned_64(8))},
  };
  View::Vector<Generic::Argument> wrong_arity;

  auto created = formula.create(accepted, arena);
  ASSERT(created);
  EXPECT(created->is<Types::View>());
  EXPECT_TEXT(created->get_name(), "View[Unsigned_8]"_view);
  EXPECT(created->visit<Types::View>(
      [&element](const Types::View& selected) {
        return &selected.get_element_type() == &element ? True : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT_NOT(formula.create(wrong_category, arena));
  EXPECT_NOT(formula.create(wrong_arity, arena));
}
