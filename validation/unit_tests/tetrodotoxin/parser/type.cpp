// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/model/environment.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/generics/access.hpp"
#include "ttx/model/types/generics/view.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx;
using namespace Validation;

static Harness ParserTypeTests = {
  .name = "Tetrodotoxin::Parser::Type"_view,
};

class NamedType final : public Ttx::Model::Type {
 public:
  constexpr explicit NamedType(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }
  constexpr auto resolve_context(View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

class SingleContext final : public Concept::Abstract {
 public:
  constexpr SingleContext(View::Bytes name, const Concept::Abstract& child)
      : name(name), child(child) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }
  constexpr auto resolve_context(View::Bytes route) const
      -> const Concept::Abstract& override {
    if (route == child.get().get_name()) {
      return child.get();
    }

    return Concept::Invalid::get_invalid();
  }

 private:
  View::Bytes name;
  Concept::Reference<Concept::Abstract> child;
};

// ValueGeneric proves that the parser follows a formula's declared signature
// without switching on a builtin name. Its materialized Type is only a stable
// observation point for the scalar values delivered by the parser.
class ValueGeneric final : public Ttx::Model::Types::Generic {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Sized"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameterization;
  }

  auto find(View::Vector<Argument> arguments) const
      -> Option<Ttx::Model::Type&> override {
    if (arguments.get_size() != 2) {
      return none;
    }

    const Unsigned_64* unsigned_value = arguments[0].find<Unsigned_64>();
    const Bool* bool_value = arguments[1].find<Bool>();
    if (unsigned_value == nullptr || bool_value == nullptr) {
      return none;
    }

    parsed_unsigned = *unsigned_value;
    parsed_bool = *bool_value;
    return materialized;
  }

  constexpr auto get_unsigned() const -> Unsigned_64 { return parsed_unsigned; }
  constexpr auto get_bool() const -> Bool { return parsed_bool; }

 private:
  inline static constexpr Static::Vector<Parameters, 2> parameterization = {{
    Parameters::Unsigned_64,
    Parameters::Bool,
  }};
  mutable NamedType materialized{"SizedValue"_view};
  mutable Unsigned_64 parsed_unsigned = 0;
  mutable Bool parsed_bool = False;
};

static auto is_none(const Option<const Ttx::Model::Type&>& selected) -> Bool {
  return selected.visit(
      [](const None&) { return True; },
      [](const Ttx::Model::Type&) { return False; });
}

static auto is_selected(
    const Option<const Ttx::Model::Type&>& selected,
    const Ttx::Model::Type& expected) -> Bool {
  return selected.visit(
      [](const None&) { return False; },
      [&expected](const Ttx::Model::Type& found) {
        return Bool(&found == &expected);
      });
}

static auto contains(View::Bytes haystack, View::Bytes needle) -> Bool {
  return Algorithm::search(haystack, needle) != Count(-1);
}

struct BuiltinCase {
  View::Bytes source;
  View::Bytes resolved_name;
};

static constexpr Static::Vector<BuiltinCase, 12> builtin_cases = {{
  {"Bool"_view, "Bool"_view},
  {"Count"_view, "Unsigned_64"_view},
  {"Real_32"_view, "Real_32"_view},
  {"Real_64"_view, "Real_64"_view},
  {"Signed_8"_view, "Signed_8"_view},
  {"Signed_16"_view, "Signed_16"_view},
  {"Signed_32"_view, "Signed_32"_view},
  {"Signed_64"_view, "Signed_64"_view},
  {"Unsigned_8"_view, "Unsigned_8"_view},
  {"Unsigned_16"_view, "Unsigned_16"_view},
  {"Unsigned_32"_view, "Unsigned_32"_view},
  {"Unsigned_64"_view, "Unsigned_64"_view},
}};

PERIMORTEM_UNIT_TEST(ParserTypeTests, resolves_builtin_types_without_context) {
  Allocator::Arena arena;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  for (Count i = 0; i < builtin_cases.get_size(); i++) {
    Lexical::Errors errors;
    Lexical::Tokenizer tokenizer(
        arena, builtin_cases[i].source, "<builtin type>"_view);
    Lexical::Cursor cursor(tokenizer, errors);

    Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);
    Bool correct_type = parsed.visit(
        [](const None&) { return False; },
        [i](const Ttx::Model::Type& found) {
          return found.get_name() == builtin_cases[i].resolved_name;
        });

    EXPECT(correct_type);
    EXPECT(errors.is_empty());
    EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
  }
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, resolves_context_type) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(arena, "Widget"_view, "<context type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_selected(parsed, widget));
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, resolves_nested_context_type) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext graphics("Graphics"_view, widget);
  SingleContext root("Root"_view, graphics);
  Lexical::Tokenizer tokenizer(
      arena, "Graphics::Widget"_view, "<nested type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_selected(parsed, widget));
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_missing_abstract_segment) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(arena, "Missing"_view, "<missing type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_non_type_abstract) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext graphics("Graphics"_view, widget);
  SingleContext root("Root"_view, graphics);
  Lexical::Tokenizer tokenizer(arena, "Graphics"_view, "<non type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, common_generics_require_environment) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(
      arena, "View[Unsigned_8]"_view, "<missing environment>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, materializes_environment_generics) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "Access[View[Unsigned_8]]"_view, "<generic type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, source);
  Bool correct = parsed.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Type& selected) {
        if (!selected.is<Ttx::Model::Types::Generics::Access::Type>()) {
          return False;
        }

        const auto& access =
            selected.assume<Ttx::Model::Types::Generics::Access::Type>();
        const Ttx::Model::Type& element = access.get_element_type();
        if (!element.is<Ttx::Model::Types::Generics::View::Type>()) {
          return False;
        }

        const auto& view =
            element.assume<Ttx::Model::Types::Generics::View::Type>();
        return view.get_element_type().get_name() == "Unsigned_8"_view;
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(
    ParserTypeTests,
    reuses_environment_materialization_across_sources) {
  Allocator::Arena arena;
  Lexical::Errors first_errors;
  Lexical::Errors second_errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source first_source(environment, {});
  Tetrodotoxin::Model::Source second_source(environment, {});
  Lexical::Tokenizer first_tokenizer(
      arena, "View[Unsigned_8]"_view, "<first generic type>"_view);
  Lexical::Tokenizer second_tokenizer(
      arena, "View[Unsigned_8]"_view, "<second generic type>"_view);
  Lexical::Cursor first_cursor(first_tokenizer, first_errors);
  Lexical::Cursor second_cursor(second_tokenizer, second_errors);

  Option<const Ttx::Model::Type&> first =
      Parser::Type::parse(first_cursor, first_source);
  Option<const Ttx::Model::Type&> second =
      Parser::Type::parse(second_cursor, second_source);
  Bool same = first.visit(
      [](const None&) { return False; },
      [&second](const Ttx::Model::Type& first_type) {
        return second.visit(
            [](const None&) { return False; },
            [&first_type](const Ttx::Model::Type& second_type) {
              return Bool(&first_type == &second_type);
            });
      });

  EXPECT(same);
  EXPECT(first_errors.is_empty());
  EXPECT(second_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, parses_value_parameter_kinds) {
  Allocator::Arena arena;
  Lexical::Errors decimal_errors;
  Lexical::Errors hex_errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer decimal_tokenizer(
      arena, "Sized[42,true]"_view, "<decimal generic>"_view);
  Lexical::Cursor decimal_cursor(decimal_tokenizer, decimal_errors);

  Option<const Ttx::Model::Type&> decimal =
      Parser::Type::parse(decimal_cursor, root);

  EXPECT(!is_none(decimal));
  EXPECT_EQ(generic.get_unsigned(), Unsigned_64(42));
  EXPECT(generic.get_bool());
  EXPECT(decimal_errors.is_empty());

  Lexical::Tokenizer hex_tokenizer(
      arena, "Sized[0x2A,false]"_view, "<hex generic>"_view);
  Lexical::Cursor hex_cursor(hex_tokenizer, hex_errors);
  Option<const Ttx::Model::Type&> hex = Parser::Type::parse(hex_cursor, root);

  EXPECT(!is_none(hex));
  EXPECT_EQ(generic.get_unsigned(), Unsigned_64(42));
  EXPECT_NOT(generic.get_bool());
  EXPECT(hex_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, reports_missing_arguments_and_recovers) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "View[]; Bool"_view, "<missing generic argument>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, source);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(contains(rendered, "Not enough arguments"_view));
  EXPECT(contains(rendered, "Missing Type"_view));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(
    ParserTypeTests,
    rejects_wrong_argument_kind_and_recovers) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "View[8]; Bool"_view, "<wrong generic argument>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, source);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_missing_separator_and_recovers) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer tokenizer(
      arena, "Sized[42 true]; Bool"_view, "<missing separator>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_extra_argument_and_recovers) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "View[Unsigned_8, Bool]; Bool"_view, "<extra argument>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, source);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_overflow_and_recovers) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Lexical::Errors errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer tokenizer(
      arena, "Sized[18446744073709551616,true]; Bool"_view,
      "<overflow generic argument>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed = Parser::Type::parse(cursor, root);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(contains(rendered, "outside Unsigned_64"_view));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}
