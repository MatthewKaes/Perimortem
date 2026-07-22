// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx;
using namespace Validation;

static Harness ParserTypeTests = {
  .name = "Tetrodotoxin::Parser::Type"_view,
};

class NamedType final : public Model::Type {
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

static auto is_none(const Option<const Model::Type&>& selected) -> Bool {
  return selected.visit(
      [](const None&) { return True; },
      [](const Model::Type&) { return False; });
}

static auto is_selected(
    const Option<const Model::Type&>& selected,
    const Model::Type& expected) -> Bool {
  return selected.visit(
      [](const None&) { return False; },
      [&expected](const Model::Type& found) {
        return Bool(&found == &expected);
      });
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

    Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);
    Bool correct_type = parsed.visit(
        [](const None&) { return False; },
        [i](const Model::Type& found) {
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

  Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);

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

  Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);

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

  Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);

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

  Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(ParserTypeTests, leaves_generic_arguments_unsupported) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  NamedType widget("Widget"_view);
  SingleContext root("Root"_view, widget);
  Lexical::Tokenizer tokenizer(
      arena, "Unsigned_8[4]"_view, "<generic type>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  Option<const Model::Type&> parsed = Parser::Type::parse(cursor, root);

  EXPECT(is_none(parsed));
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(cursor.matches(Lexical::Code::Type::LayoutStart));
}
