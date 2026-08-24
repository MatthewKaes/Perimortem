// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/definition.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Language;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class DefinitionDialect : public Dialect {
 public:
  DefinitionDialect() : Dialect("Definition"_view) {}

  auto interpret(Cursor&, const Documentation&, const Anchor&, Abstract&)
      -> Option<Monograph&> override {
    return {};
  }
};

class DefinitionHost : public Monograph {
 public:
  DefinitionHost(Allocator::Arena& arena, Dialect& dialect)
      : Monograph(arena, dialect, Documentation::get_empty(), dialect) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "DefinitionHost"_view;
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static Harness DefinitionTests = {
  .name = "Tetrodotoxin::Language::Definition"_view,
};

PERIMORTEM_UNIT_TEST(DefinitionTests, complete_prefix) {
  static constexpr View::Bytes source =
      "// Shared definition.\n"
      "@first @first(2) public state worker : func = [] -> [] {}"_view;
  Allocator::Arena arena;
  DefinitionDialect dialect;
  DefinitionHost host(arena, dialect);
  Errors errors;
  View::Bytes retained_source = arena.proxy(source);
  View::Bytes retained_path = arena.proxy("<definition>"_view);
  Tokenizer& tokenizer =
      arena.construct<Tokenizer>(arena, retained_source, retained_path);
  Associations& associations = arena.construct<Associations>(arena);
  Cursor& cursor = arena.construct<Cursor>(tokenizer, errors, associations);

  const Documentation& documentation = Parser::Comment::parse(cursor);
  auto definition = Definition::parse(cursor, documentation, host);

  ASSERT(definition);
  EXPECT_TEXT(definition->get_name(), "worker"_view);
  EXPECT(&definition->get_host() == &host);
  EXPECT(definition->get_qualifier().get_code() == Code::Type::Func);
  EXPECT(definition->get_visibility() == Visibility::Public);
  EXPECT(definition->get_visibility_token().get_code() == Code::Type::Public);
  EXPECT_TEXT(
      definition->get_visibility_token().caculate_text(source), "public"_view);
  ASSERT_EQ(definition->get_modifiers().get_size(), Count(1));
  EXPECT(
      definition->get_modifiers().get_data()[0].get_code() ==
      Code::Type::State);
  ASSERT_EQ(definition->get_attributes().get_size(), Count(2));
  EXPECT_TEXT(
      definition->get_attributes().get_data()[0].get_key(), "first"_view);
  EXPECT_TEXT(
      definition->get_attributes().get_data()[1].get_key(), "first"_view);
  EXPECT_TEXT(
      definition->get_documentation().get_line(0), "Shared definition."_view);
  EXPECT_TEXT(
      definition->get_name_anchor().get_span().caculate_text(source),
      "worker"_view);
  EXPECT_TEXT(
      definition->get_anchor().get_span().caculate_text(source),
      "first @first(2) public state worker : func"_view);
  EXPECT(cursor.matches(Code::Type::Func));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DefinitionTests, type_qualifier) {
  Allocator::Arena arena;
  DefinitionDialect dialect;
  DefinitionHost host(arena, dialect);
  Errors errors;
  View::Bytes retained_source = arena.proxy("private value : U64 = 1;"_view);
  View::Bytes retained_path = arena.proxy("<typed definition>"_view);
  Tokenizer& tokenizer =
      arena.construct<Tokenizer>(arena, retained_source, retained_path);
  Associations& associations = arena.construct<Associations>(arena);
  Cursor& cursor = arena.construct<Cursor>(tokenizer, errors, associations);

  const Documentation& documentation = Parser::Comment::parse(cursor);
  auto definition =
      Definition::parse(cursor, documentation, host, View::Vector<Attribute>());

  ASSERT(definition);
  EXPECT_TEXT(definition->get_name(), "value"_view);
  EXPECT(definition->get_visibility() == Visibility::Private);
  EXPECT(definition->get_qualifier().get_code() == Code::Type::Type);
  EXPECT(definition->get_attributes().is_empty());
  EXPECT(cursor.matches(Code::Type::Type));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DefinitionTests, malformed_prefix) {
  Allocator::Arena arena;
  DefinitionDialect dialect;
  DefinitionHost host(arena, dialect);
  Errors errors;
  View::Bytes retained_source =
      arena.proxy("@note public private value : U64 = 1;"_view);
  View::Bytes retained_path = arena.proxy("<malformed definition>"_view);
  Tokenizer& tokenizer =
      arena.construct<Tokenizer>(arena, retained_source, retained_path);
  Associations& associations = arena.construct<Associations>(arena);
  Cursor& cursor = arena.construct<Cursor>(tokenizer, errors, associations);

  const Documentation& documentation = Parser::Comment::parse(cursor);
  auto definition = Definition::parse(cursor, documentation, host);

  EXPECT(!definition);
  EXPECT(!errors.is_empty());
}
