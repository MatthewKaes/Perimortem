// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/layout.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LibraryModelLayout = {
  .name = "Tetrodotoxin::Library::Language::Model::Layout"_view,
};

static auto interpret_source(Workspace& workspace, Errors& errors)
    -> Option<Language::Monograph&> {
  BAIL_IF(!workspace.install_dialect<Dialect>("Library"_view));

  auto interpreted = workspace.interpret_source(
      errors, "LayoutModelTest"_view, "layout-model.ttx"_view,
      "// Authored Layout model test.\n"
      "dialect : Library;\n"
      "public Box : struct { public value : Bool; }"_view);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpreted);
}

static auto parse_layout(
    Allocator::Arena& arena,
    Language::Monograph& source,
    Errors& errors,
    View::Bytes text,
    Bool parameters = False) -> Option<Language::Model::Layout&> {
  Tokenizer tokenizer(arena, text, "authored-layout.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto layout =
      parameters
          ? Language::Model::Layout::interpret_parameters(arena, source, cursor)
          : Language::Model::Layout::interpret(arena, source, cursor);
  BAIL_IF(!layout || !cursor.matches(Code::Type::Terminal));
  return *layout;
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, owns_parameter_entries) {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  const Abstract& box = monograph->get_source().resolve_context("Box"_view);
  ASSERT(box.is<Language::Types::Composite>());

  Allocator::Arena arena;
  Errors parse_errors;
  auto layout = parse_layout(
      arena, *monograph, parse_errors, "[self, .input : Bool,]"_view, True);
  ASSERT(layout);
  EXPECT(layout->declares_self());
  EXPECT_NOT(layout->is_linked());
  ASSERT(layout->link_parameters(
      *monograph, static_cast<const Ttx::Model::Type&>(box)));

  ASSERT_EQ(layout->get_size(), Count(2));
  ASSERT(layout->get_name(0) && layout->get_name(1));
  EXPECT_TEXT(*layout->get_name(0), "self"_view);
  EXPECT_TEXT(*layout->get_name(1), "input"_view);

  const Abstract& self = layout->resolve_named("self"_view);
  const Abstract& input = layout->resolve_named("input"_view);
  ASSERT(self.is<Language::Parameter>());
  ASSERT(input.is<Language::Parameter>());
  EXPECT(&static_cast<const Language::Parameter&>(self).get_type() == &box);
  EXPECT(
      &static_cast<const Language::Parameter&>(input).get_type() ==
      &Dialect::get_bool());
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, empty_types_remain_zero_value_flow) {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  Allocator::Arena arena;
  Errors parse_errors;
  auto empty = parse_layout(arena, *monograph, parse_errors, "[]"_view);
  auto authored_void =
      parse_layout(arena, *monograph, parse_errors, "Void"_view);
  auto named = parse_layout(
      arena, *monograph, parse_errors,
      "[.nothing : Void, .value : Bool,]"_view);
  ASSERT(empty && authored_void && named);

  EXPECT(empty->is_linked());
  EXPECT_NOT(authored_void->is_linked());
  EXPECT_NOT(named->is_linked());
  ASSERT(authored_void->link_types(*monograph, monograph->get_source()));
  ASSERT(named->link_types(*monograph, monograph->get_source()));

  EXPECT(empty->is_empty());
  EXPECT(authored_void->is_empty());
  EXPECT(empty->fits(*authored_void));
  EXPECT(authored_void->fits(*empty));

  ASSERT_EQ(named->get_size(), Count(1));
  ASSERT(named->get_name(0));
  EXPECT_TEXT(*named->get_name(0), "value"_view);
  ASSERT(named->get_abstract(0));
  EXPECT(&*named->get_abstract(0) == &Dialect::get_bool());
  EXPECT(&named->resolve_named("nothing"_view) == &Invalid::get_invalid());
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, named_fitting_preserves_real_edges) {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  Allocator::Arena arena;
  Errors parse_errors;
  auto source = parse_layout(
      arena, *monograph, parse_errors,
      "[.flag : Bool, .count : Unsigned_64]"_view);
  auto reordered = parse_layout(
      arena, *monograph, parse_errors,
      "[.count : Unsigned_64, .flag : Bool]"_view);
  ASSERT(source && reordered);
  ASSERT(source->link_types(*monograph, monograph->get_source()));
  ASSERT(reordered->link_types(*monograph, monograph->get_source()));

  EXPECT(source->fits(*reordered));
  auto count = source->get_fitted(*reordered, 0);
  auto flag = source->get_fitted(*reordered, 1);
  EXPECT(count.visit(
      [](const Abstract& selected) -> Bool {
        return Bool(&selected == &Dialect::get_unsigned_64());
      },
      [](Ttx::Concept::Layout::Errors) { return False; }));
  EXPECT(flag.visit(
      [](const Abstract& selected) -> Bool {
        return Bool(&selected == &Dialect::get_bool());
      },
      [](Ttx::Concept::Layout::Errors) { return False; }));
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}
