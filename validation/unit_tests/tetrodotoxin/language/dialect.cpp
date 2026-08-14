// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/dialect.hpp"

#include "validation/unit_test.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class DefaultDialect : public Language::Dialect {
 public:
  DefaultDialect() = default;

  auto interpret(
      Allocator::Arena&,
      Cursor&,
      const Documentation&,
      const Anchor&,
      Language::Diagnostics&,
      Abstract&) -> Option<Language::Monograph&> override {
    return {};
  }
};

class DefaultMonograph : public Language::Monograph {
 public:
  DefaultMonograph(Allocator::Arena& domain)
      : Monograph(domain, Documentation::get_empty()) {}

  DefaultMonograph(Allocator::Arena& domain, Language::Diagnostics& diagnostics)
      : Monograph(domain, Documentation::get_empty(), diagnostics) {}

  auto get_name() const -> View::Bytes override { return "Default"_view; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class EmptyEncodingDialect : public DefaultDialect {
 public:
  EmptyEncodingDialect() = default;

  auto encode(const Language::Monograph&) const
      -> Option<Dynamic::Bytes> override {
    return Dynamic::Bytes();
  }
};

static Harness LanguageDialect = {
  .name = "Tetrodotoxin::Language::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(LanguageDialect, ordered_diagnostics_are_stable) {
  Unsigned_8 message[] = {'f', 'i', 'r', 's', 't'};
  Unsigned_8 hint[] = {'h', 'i', 'n', 't'};
  Allocator::Arena arena;
  DefaultMonograph monograph(arena);
  Token opening(2, 1, 2, 1, Code::Type::Addressable);
  Token closing(8, 1, 8, 1, Code::Type::Addressable);
  Span span(opening, closing);

  monograph.report(
      Anchor::create(opening, span), View::Bytes(message), View::Bytes(hint));
  message[0] = 'x';
  hint[0] = 'x';
  monograph.report(Anchor::create(Span(closing)), "second"_view);

  View::Vector<Language::Diagnostic> diagnostics = monograph.get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(2));
  const Language::Diagnostic& first = diagnostics.get_data()[0];
  const Language::Diagnostic& second = diagnostics.get_data()[1];
  ASSERT(first.get_anchor());
  ASSERT(second.get_anchor());
  EXPECT_EQ(first.get_anchor()->get_token().get_offset(), opening.get_offset());
  EXPECT_EQ(first.get_anchor()->get_span().get_offset(), span.get_offset());
  EXPECT_EQ(first.get_anchor()->get_span().get_size(), span.get_size());
  EXPECT_TEXT(first.get_message(), "first"_view);
  EXPECT_TEXT(first.get_hint(), "hint"_view);
  EXPECT_EQ(
      second.get_anchor()->get_span().get_offset(),
      Count(closing.get_offset()));
  EXPECT_TEXT(second.get_message(), "second"_view);
  EXPECT(second.get_hint().is_empty());
}

PERIMORTEM_UNIT_TEST(LanguageDialect, shared_diagnostic_transaction) {
  Allocator::Arena arena;
  Language::Diagnostics outer_diagnostics(arena);
  Language::Diagnostics isolated_diagnostics(arena);
  DefaultMonograph outer(arena, outer_diagnostics);
  DefaultMonograph child(arena, outer_diagnostics);
  DefaultMonograph isolated(arena, isolated_diagnostics);
  Token opening(4, 1, 5, 3, Code::Type::Addressable);

  outer.report(Anchor::create(Span(opening)), "parse failure"_view);
  child.report({}, "child link failure"_view, "child hint"_view);
  outer.report({}, "finalize failure"_view);
  isolated.report({}, "isolated failure"_view);

  View::Vector<Language::Diagnostic> outer_values = outer.get_diagnostics();
  View::Vector<Language::Diagnostic> child_values = child.get_diagnostics();
  View::Vector<Language::Diagnostic> isolated_values =
      isolated.get_diagnostics();
  ASSERT_EQ(outer_values.get_size(), Count(3));
  ASSERT_EQ(child_values.get_size(), Count(3));
  ASSERT_EQ(isolated_values.get_size(), Count(1));
  EXPECT(&outer_values.get_data()[0] == &child_values.get_data()[0]);
  EXPECT(outer_values.get_data()[0].get_anchor());
  EXPECT_NOT(outer_values.get_data()[1].get_anchor());
  EXPECT_TEXT(outer_values.get_data()[0].get_message(), "parse failure"_view);
  EXPECT_TEXT(
      outer_values.get_data()[1].get_message(), "child link failure"_view);
  EXPECT_TEXT(outer_values.get_data()[1].get_hint(), "child hint"_view);
  EXPECT_TEXT(
      outer_values.get_data()[2].get_message(), "finalize failure"_view);
  EXPECT_TEXT(
      isolated_values.get_data()[0].get_message(), "isolated failure"_view);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, explicit_default_persistence) {
  Allocator::Arena arena;
  DefaultDialect dialect;
  DefaultMonograph monograph(arena);
  EmptyEncodingDialect empty_dialect;
  DefaultMonograph empty_monograph(arena);

  const Bool linked = monograph.link();
  const Bool finalized = monograph.finalize();
  auto unsupported = dialect.encode(monograph);
  auto missing = dialect.restore(arena, "unsupported"_view);
  auto empty = empty_dialect.encode(empty_monograph);
  Bool successful_empty = empty.visit(
      []() { return False; },
      [](const Dynamic::Bytes& payload) {
        return payload.is_empty() ? True : False;
      });

  EXPECT(linked);
  EXPECT(finalized);
  EXPECT_NOT(monograph.is<Ttx::Model::Type>());
  EXPECT(&monograph.resolve_context("source"_view) == &Invalid::get_invalid());
  EXPECT_NOT(unsupported);
  EXPECT_NOT(missing);
  EXPECT(successful_empty);
}
