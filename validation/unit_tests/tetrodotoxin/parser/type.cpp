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
#include "ttx/model/types/generics/fixed.hpp"
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
  class Type final : public Ttx::Model::Type {
   public:
    using ContractOwner = Type;
    static constexpr Perimortem::System::Uuid contract_id{
      0x8f2cd080186e4a95,
      0xaee8d98b3fda7eba,
    };

    constexpr Type(Unsigned_64 unsigned_value, Bool bool_value)
        : unsigned_value(unsigned_value), bool_value(bool_value) {}

    constexpr auto implements(Perimortem::System::Uuid requested) const
        -> Bool override {
      return requested == contract_id ||
             Ttx::Model::Type::implements(requested);
    }

    constexpr auto get_name() const -> View::Bytes override {
      return "SizedValue"_view;
    }
    constexpr auto get_documentation() const
        -> const Concept::Documentation& override {
      return Concept::Documentation::get_empty();
    }
    constexpr auto resolve_context(View::Bytes) const
        -> const Concept::Abstract& override {
      return Concept::Invalid::get_invalid();
    }

    constexpr auto get_unsigned() const -> Unsigned_64 {
      return unsigned_value;
    }
    constexpr auto get_bool() const -> Bool { return bool_value; }

   private:
    Unsigned_64 unsigned_value;
    Bool bool_value;
  };

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

 private:
  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Option<const Ttx::Model::Type&> override {
    if (arguments.get_size() != 2) {
      return none;
    }

    const Unsigned_64* unsigned_value = arguments[0].find<Unsigned_64>();
    const Bool* bool_value = arguments[1].find<Bool>();
    if (unsigned_value == nullptr || bool_value == nullptr) {
      return none;
    }

    return arena.construct<Type>(*unsigned_value, *bool_value);
  }

  static constexpr Static::Vector<Parameters, 2> parameterization = {{
    Parameters::Unsigned_64,
    Parameters::Bool,
  }};
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
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  for (Count i = 0; i < builtin_cases.get_size(); i++) {
    Lexical::Errors errors;
    Lexical::Tokenizer tokenizer(
        arena, builtin_cases[i].source, "<builtin type>"_view);
    Lexical::Cursor cursor(tokenizer, errors);

    Option<const Ttx::Model::Type&> parsed =
        Parser::Type::parse(cursor, root, materializations);
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
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(arena, "Widget"_view, "<context type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

  EXPECT(is_selected(parsed, widget));
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, resolves_nested_context_type) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext graphics("Graphics"_view, widget);
  SingleContext root("Root"_view, graphics);
  Lexical::Tokenizer tokenizer(
      arena, "Graphics::Widget"_view, "<nested type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

  EXPECT(is_selected(parsed, widget));
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_missing_abstract_segment) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(arena, "Missing"_view, "<missing type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_non_type_abstract) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext graphics("Graphics"_view, widget);
  SingleContext root("Root"_view, graphics);
  Lexical::Tokenizer tokenizer(arena, "Graphics"_view, "<non type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, common_generics_require_environment) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(
      arena, "View[Unsigned_8]"_view, "<missing environment>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

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

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());
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

PERIMORTEM_UNIT_TEST(ParserTypeTests, materializes_fixed_ranges) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "Fixed[Unsigned_8, 4]"_view, "<fixed generic type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());
  Bool correct = parsed.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Type& selected) -> Bool {
        if (!selected.is<Ttx::Model::Types::Generics::Fixed::Type>()) {
          return False;
        }

        const auto& fixed =
            selected.assume<Ttx::Model::Types::Generics::Fixed::Type>();
        return fixed.get_element_type().get_name() == "Unsigned_8"_view &&
               fixed.get_extent() == Signed_64(4) &&
               fixed.get_layout().get_size() == Count(4);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, materializes_recursive_fixed_ranges) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "Fixed[Fixed[Fixed[Unsigned_8, 2], 3], 4]"_view,
      "<recursive fixed generic type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());
  Bool correct = parsed.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Type& selected) -> Bool {
        if (!selected.is<Ttx::Model::Types::Generics::Fixed::Type>()) {
          return False;
        }

        const auto& outer =
            selected.assume<Ttx::Model::Types::Generics::Fixed::Type>();
        const Ttx::Model::Type& middle_type = outer.get_element_type();
        if (!middle_type.is<Ttx::Model::Types::Generics::Fixed::Type>()) {
          return False;
        }

        const auto& middle =
            middle_type.assume<Ttx::Model::Types::Generics::Fixed::Type>();
        const Ttx::Model::Type& inner_type = middle.get_element_type();
        if (!inner_type.is<Ttx::Model::Types::Generics::Fixed::Type>()) {
          return False;
        }

        const auto& inner =
            inner_type.assume<Ttx::Model::Types::Generics::Fixed::Type>();
        return outer.get_extent() == Signed_64(4) &&
               outer.get_layout().get_size() == Count(4) &&
               middle.get_extent() == Signed_64(3) &&
               middle.get_layout().get_size() == Count(3) &&
               inner.get_extent() == Signed_64(2) &&
               inner.get_layout().get_size() == Count(2) &&
               inner.get_element_type().get_name() == "Unsigned_8"_view;
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(
    ParserTypeTests,
    recovers_after_recursive_generic_argument_failure) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena,
      "Fixed[Fixed[Fixed[Unsigned_8, 2], Unsigned_16], 4]; "
      "Fixed[Real_32, 2]"_view,
      "<recursive fixed recovery>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> rejected =
      Parser::Type::parse(cursor, source, environment.get_materializations());
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT(is_none(rejected));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(contains(rendered, "expected Signed_64"_view));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Fixed"_view);

  Option<const Ttx::Model::Type&> recovered =
      Parser::Type::parse(cursor, source, environment.get_materializations());
  Bool correct = recovered.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Type& selected) -> Bool {
        if (!selected.is<Ttx::Model::Types::Generics::Fixed::Type>()) {
          return False;
        }

        const auto& fixed =
            selected.assume<Ttx::Model::Types::Generics::Fixed::Type>();
        return fixed.get_element_type().get_name() == "Real_32"_view &&
               fixed.get_extent() == Signed_64(2) &&
               fixed.get_layout().get_size() == Count(2);
      });

  EXPECT(correct);
  EXPECT_EQ(errors.get_size(), Count(1));
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

  Option<const Ttx::Model::Type&> first = Parser::Type::parse(
      first_cursor, first_source, environment.get_materializations());
  Option<const Ttx::Model::Type&> second = Parser::Type::parse(
      second_cursor, second_source, environment.get_materializations());
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
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors decimal_errors;
  Lexical::Errors hex_errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer decimal_tokenizer(
      arena, "Sized[42,true]"_view, "<decimal generic>"_view);
  Lexical::Cursor decimal_cursor(decimal_tokenizer, decimal_errors);

  Option<const Ttx::Model::Type&> decimal =
      Parser::Type::parse(decimal_cursor, root, materializations);
  const auto& decimal_type = decimal.visit(
      [](const None&) -> const ValueGeneric::Type& { __builtin_unreachable(); },
      [](const Ttx::Model::Type& type) -> const ValueGeneric::Type& {
        return type.assume<ValueGeneric::Type>();
      });

  EXPECT(!is_none(decimal));
  EXPECT_EQ(decimal_type.get_unsigned(), Unsigned_64(42));
  EXPECT(decimal_type.get_bool());
  EXPECT(decimal_errors.is_empty());

  Lexical::Tokenizer hex_tokenizer(
      arena, "Sized[0x2A,false]"_view, "<hex generic>"_view);
  Lexical::Cursor hex_cursor(hex_tokenizer, hex_errors);
  Option<const Ttx::Model::Type&> hex =
      Parser::Type::parse(hex_cursor, root, materializations);
  const auto& hex_type = hex.visit(
      [](const None&) -> const ValueGeneric::Type& { __builtin_unreachable(); },
      [](const Ttx::Model::Type& type) -> const ValueGeneric::Type& {
        return type.assume<ValueGeneric::Type>();
      });

  EXPECT(!is_none(hex));
  EXPECT_EQ(hex_type.get_unsigned(), Unsigned_64(42));
  EXPECT_NOT(hex_type.get_bool());
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

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());
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

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_missing_separator_and_recovers) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Lexical::Errors errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer tokenizer(
      arena, "Sized[42 true]; Bool"_view, "<missing separator>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);

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

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_overflow_and_recovers) {
  Allocator::Arena arena;
  Ttx::Model::Types::Generic::Materializations materializations(arena);
  Allocator::Arena render_arena;
  Lexical::Errors errors;
  ValueGeneric generic;
  SingleContext root("Root"_view, generic);
  Lexical::Tokenizer tokenizer(
      arena, "Sized[18446744073709551616,true]; Bool"_view,
      "<overflow generic argument>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, root, materializations);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(contains(rendered, "outside Unsigned_64"_view));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(
    ParserTypeTests,
    rejects_negative_fixed_extent_and_recovers) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "Fixed[Unsigned_8,-1]; Bool"_view, "<negative fixed extent>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, rejects_signed_overflow_and_recovers) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Lexical::Errors errors;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {});
  Lexical::Tokenizer tokenizer(
      arena, "Fixed[Unsigned_8,9223372036854775808]; Bool"_view,
      "<overflow fixed extent>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Ttx::Model::Type&> parsed =
      Parser::Type::parse(cursor, source, environment.get_materializations());
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(contains(rendered, "outside Signed_64"_view));
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Bool"_view);
}
