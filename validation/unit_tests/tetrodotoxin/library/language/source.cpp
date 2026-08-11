// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/source.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Validation;

class SourceAddressable : public Addressable {
 public:
  constexpr SourceAddressable(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  constexpr auto get_type() const -> const Type& override {
    return Dialect::get_bool();
  }

 private:
  View::Bytes name;
};

class SourceContext : public Abstract {
 public:
  constexpr SourceContext(const Abstract& selected) : selected(selected) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "SourceContext"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return route == "item"_view ? selected : Invalid::get_invalid();
  }

 private:
  const Abstract& selected;
};

class LateSourceContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "LateSourceContext"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "item"_view && selected != nullptr) {
      return *selected;
    }

    return Invalid::get_invalid();
  }

  constexpr auto publish(const Abstract& value) -> void { selected = &value; }

 private:
  const Abstract* selected = nullptr;
};

class SourceMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  SourceMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "SourceMonograph"_view;
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static Harness SourceTests = {
  .name = "Tetrodotoxin::Library::Language::Source"_view,
};

static_assert(
    __is_base_of(Language::Types::Composite, Language::Types::Source));
static_assert(
    !__is_base_of(Language::Types::Structure, Language::Types::Source));

PERIMORTEM_UNIT_TEST(SourceTests, literal_anchors) {
  static constexpr View::Bytes source = "true -5 \"xy\""_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  SourceContext context(Invalid::get_invalid());
  Errors errors;
  Tokenizer tokenizer(arena, source, "literal-anchors.ttx"_view);
  Cursor cursor(tokenizer, errors);

  auto flag = Language::Parser::Literal::parse(
      arena, materializations, cursor, context);
  auto signed_value = Language::Parser::Literal::parse(
      arena, materializations, cursor, context);
  auto bytes = Language::Parser::Literal::parse(
      arena, materializations, cursor, context);
  ASSERT(flag && signed_value && bytes);
  ASSERT(flag->get_anchor());
  ASSERT(signed_value->get_anchor());
  ASSERT(bytes->get_anchor());

  // Focus Tokens mark each literal grammar while full Spans retain every Token
  // that contributed to the semantic value.
  EXPECT_TEXT(
      flag->get_anchor()->get_token().caculate_text(source), "true"_view);
  EXPECT_TEXT(
      flag->get_anchor()->get_span().caculate_text(source), "true"_view);
  EXPECT_TEXT(
      signed_value->get_anchor()->get_token().caculate_text(source), "-"_view);
  EXPECT_TEXT(
      signed_value->get_anchor()->get_span().caculate_text(source), "-5"_view);
  EXPECT_TEXT(
      bytes->get_anchor()->get_token().caculate_text(source), "\"xy\""_view);
  EXPECT_TEXT(
      bytes->get_anchor()->get_span().caculate_text(source), "\"xy\""_view);
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());

  auto& generated =
      Language::Constants::True::create_synthetic(arena, Dialect::get_bool());
  auto& generated_operation = Language::Operations::Not::create_synthetic(
      arena, materializations, generated);
  EXPECT_NOT(generated.get_anchor());
  EXPECT_NOT(generated_operation.get_anchor());
}

PERIMORTEM_UNIT_TEST(SourceTests, identifier_links_once) {
  static constexpr View::Bytes source = "item"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  SourceAddressable first("item"_view);
  SourceAddressable second("item"_view);
  SourceContext first_context(first);
  SourceContext second_context(second);
  SourceMonograph graph(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "identifier.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto expression = Language::Parser::Expression::parse(
      arena, materializations, cursor, first_context);
  ASSERT(expression);
  Language::Identifier* identifier = expression->visit<Language::Identifier>(
      [](Language::Identifier& selected) { return &selected; },
      [](Abstract&) -> Language::Identifier* { return nullptr; });
  ASSERT(identifier);
  const auto& anchor = identifier->get_anchor();
  ASSERT(anchor);

  // An unlinked route remains a real source node with no fabricated edge.
  EXPECT_TEXT(identifier->get_name(), "item"_view);
  EXPECT_TEXT(anchor->get_token().caculate_text(source), "item"_view);
  EXPECT_TEXT(anchor->get_span().caculate_text(source), "item"_view);
  EXPECT_NOT(identifier->get_addressable());
  EXPECT(&identifier->get_type() == &Invalid::get_invalid());

  // Even a resolvable route stays untouched during parsing. Linking enriches
  // this same source node and rejects a competing later identity.
  ASSERT(identifier->link(graph, first_context, materializations));
  ASSERT(identifier->link(graph, first_context, materializations));
  EXPECT_NOT(identifier->link(graph, second_context, materializations));
  auto addressable = identifier->get_addressable();
  ASSERT(addressable);
  EXPECT(&*addressable == &first);
  EXPECT(&identifier->get_type() == &Dialect::get_bool());
  ASSERT_EQ(graph.get_diagnostics().get_size(), Count(1));
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SourceTests, late_addressable_enriches_authored_identity) {
  static constexpr View::Bytes source = "item"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  SourceAddressable addressable("item"_view);
  LateSourceContext context;
  SourceMonograph graph(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "late-addressable.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto expression = Language::Parser::Expression::parse(
      arena, materializations, cursor, context);
  ASSERT(expression);
  const Language::Expression* expression_identity = &*expression;
  Language::Identifier* identifier = expression->visit<Language::Identifier>(
      [](Language::Identifier& selected) { return &selected; },
      [](Abstract&) -> Language::Identifier* { return nullptr; });
  ASSERT(identifier);

  EXPECT_NOT(identifier->get_addressable());
  EXPECT(&identifier->get_type() == &Invalid::get_invalid());

  context.publish(addressable);
  ASSERT(identifier->link(graph, context, materializations));
  EXPECT(&*expression == expression_identity);
  auto linked = identifier->get_addressable();
  ASSERT(linked);
  EXPECT(&*linked == &addressable);
  EXPECT(&identifier->get_type() == &Dialect::get_bool());
  EXPECT(graph.get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SourceTests, import_extent) {
  static constexpr View::Bytes source = "using Core::Math; trailing"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "import-extent.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto import = Language::Import::parse(cursor);
  ASSERT(import);

  EXPECT_TEXT(import->get_route(), "Core::Math"_view);
  EXPECT_TEXT(import->get_token().caculate_text(source), "using"_view);
  EXPECT_TEXT(
      import->get_span().caculate_text(source), "using Core::Math;"_view);
  EXPECT_TEXT(cursor.current().caculate_text(source), "trailing"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SourceTests, authored_declaration_order) {
  static constexpr View::Bytes source =
      "using First;\n"
      "public body : func = [] -> Void { true; }\n"
      "using Second;"_view;
  Allocator::Arena arena;
  SourceContext context(Invalid::get_invalid());
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "declaration-order.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), context);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  const auto& monograph = static_cast<const Language::Monograph&>(*interpreted);

  // Imports retain their authored sequence beside the shared declaration
  // inventory. The synthetic source owns lookup without replacing either
  // source relation with selected edges.
  auto imports = monograph.get_imports();
  auto bindings = monograph.get_authored_bindings();
  ASSERT_EQ(imports.get_size(), Count(2));
  ASSERT_EQ(bindings.get_size(), Count(1));
  EXPECT_TEXT(imports.get_data()[0].get_route(), "First"_view);
  EXPECT_TEXT(imports.get_data()[1].get_route(), "Second"_view);
  EXPECT_TEXT(
      imports.get_data()[0].get_span().caculate_text(source),
      "using First;"_view);
  EXPECT_TEXT(
      imports.get_data()[1].get_span().caculate_text(source),
      "using Second;"_view);
  ASSERT(bindings.get_data()[0].get().is<Language::Function>());
  const auto& function =
      static_cast<const Language::Function&>(bindings.get_data()[0].get());
  EXPECT_TEXT(function.get_name(), "body"_view);
  EXPECT_TEXT(
      function.get_span().caculate_text(source),
      "public body : func = [] -> Void { true; }"_view);
  ASSERT_EQ(function.get_expressions().get_size(), Count(1));
  const Language::Expression& expression =
      function.get_expressions().get_data()[0].get();
  ASSERT(expression.get_anchor());
  EXPECT_TEXT(
      expression.get_anchor()->get_span().caculate_text(source), "true"_view);
  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_type =
      static_cast<const Language::Types::Source&>(monograph.get_source());
  EXPECT(source_type.is<Language::Types::Composite>());
  EXPECT_NOT(source_type.is<Language::Types::Structure>());
  EXPECT_TEXT(source_type.get_name(), "source"_view);
  EXPECT(&source_type.get_documentation() == &monograph.get_documentation());
  EXPECT_NOT(source_type.get_anchor());
  EXPECT(source_type.get_layout().is_empty());
  EXPECT(&monograph.resolve_context("source"_view) == &source_type);
  EXPECT(&function.get_source() == &monograph);
  EXPECT(&function.get_host() == &source_type);
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}
