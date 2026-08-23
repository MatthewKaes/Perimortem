// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class DefaultDialect : public Language::Dialect {
 public:
  DefaultDialect(View::Bytes name = "Default"_view) : Language::Dialect(name) {}

  auto interpret(Cursor&, const Documentation&, const Anchor&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }
};

class DefaultMonograph : public Language::Monograph {
 public:
  DefaultMonograph(
      Allocator::Arena& arena,
      const Language::Dialect& dialect,
      Abstract& context)
      : Monograph(arena, dialect, Documentation::get_empty(), context) {}

  auto get_name() const -> View::Bytes override { return "Default"_view; }
};

class Context final : public Abstract {
 public:
  TTX_CONTRACT(Context, Abstract);

  auto get_name() const -> View::Bytes override { return "Context"_view; }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "provided"_view) {
      return *this;
    }
    return Invalid::get_invalid();
  }
};

class EmptyEncodingDialect final : public DefaultDialect {
 public:
  EmptyEncodingDialect() : DefaultDialect("EmptyEncoding"_view) {}

  auto encode(const Abstract&, Language::Persistence::Profile) const
      -> Option<Dynamic::Bytes> override {
    return Dynamic::Bytes();
  }
};

static Harness LanguageDialect = {
  .name = "Tetrodotoxin::Language::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(LanguageDialect, explicit_defaults) {
  Allocator::Arena arena;
  DefaultDialect dialect;
  Context context;
  DefaultMonograph monograph(arena, dialect, context);
  EmptyEncodingDialect empty_dialect;
  DefaultMonograph empty_monograph(arena, empty_dialect, context);
  Errors errors;
  Tokenizer tokenizer(arena, {}, "default.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  const Bool linked = monograph.link(cursor);
  const Bool finalized = monograph.finalize(cursor);
  auto unsupported =
      dialect.encode(monograph, Language::Persistence::Profile::Complete);
  auto missing = dialect.restore(
      arena, "unsupported"_view, Language::Persistence::Profile::Complete,
      Documentation::get_empty(), context);
  auto empty = empty_dialect.encode(
      empty_monograph, Language::Persistence::Profile::Complete);
  Bool successful_empty = empty.visit(
      []() { return False; },
      [](const Dynamic::Bytes& payload) {
        return payload.is_empty() ? True : False;
      });

  EXPECT(linked);
  EXPECT(finalized);
  EXPECT_NOT(monograph.is<Ttx::Model::Type>());
  EXPECT_NOT(unsupported);
  EXPECT_NOT(missing);
  EXPECT(successful_empty);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LanguageDialect, exact_layer_identity) {
  Allocator::Arena arena;
  DefaultDialect installed("Installed"_view);
  DefaultDialect same_type("SameType"_view);
  Context context;
  DefaultMonograph monograph(arena, installed, context);

  auto selected = monograph.get_layer(installed);
  auto rejected = monograph.get_layer(same_type);

  ASSERT(selected);
  EXPECT(&*selected == &monograph);
  EXPECT(&monograph.get_language() == &installed);
  EXPECT_NOT(rejected);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, parent_context) {
  Allocator::Arena arena;
  DefaultDialect installed("Installed"_view);
  Context context;
  DefaultMonograph monograph(arena, installed, context);

  EXPECT(installed.is<Language::Dialect>());
  EXPECT(&monograph.get_language() == &installed);
  EXPECT(&monograph.resolve_context("provided"_view) == &context);
  EXPECT(&monograph.resolve_context("missing"_view) == &Invalid::get_invalid());
}
