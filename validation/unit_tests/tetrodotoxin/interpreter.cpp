// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/interpreter/defines.hpp"
#include "tetrodotoxin/interpreter/dialects/alias.hpp"
#include "tetrodotoxin/interpreter/dialects/group.hpp"
#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/group.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

/// An address-bearing fixture proves Alias routes are not restricted to Types.
class PackageValue final : public Addressable {
 public:
  PackageValue(View::Bytes name, const Abstract& value)
      : name(name), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve() const -> const Abstract& override { return value.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return value.resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& value;
};

static Harness DialectTests = {
  .name = "Dialect"_view,
};

PERIMORTEM_UNIT_TEST(DialectTests, dialect_lookup) {
  Tetrodotoxin::Dialects::Package package;
  const Reference<Abstract> installed[] = {package};
  Group dialects("Dialects"_view, installed);

  const Abstract& selected = dialects.resolve_context("Package"_view);

  EXPECT(selected.is<Tetrodotoxin::Dialect>());
  EXPECT(selected.is<Tetrodotoxin::Dialects::Package>());
  EXPECT(&selected == &package);
  EXPECT(dialects.resolve_context("Missing"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(DialectTests, duplicate_dialect) {
  Tetrodotoxin::Dialects::Package first;
  Tetrodotoxin::Dialects::Package second;
  const Reference<Abstract> installed[] = {first, second};
  Group dialects("Dialects"_view, installed);

  EXPECT(dialects.resolve_context("Package"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(DialectTests, dialect_parts) {
  Tetrodotoxin::Dialects::Alias alias;
  Tetrodotoxin::Dialects::Group group;
  const Reference<Tetrodotoxin::Dialect> parts[] = {alias, group};
  const Ttx::Lexical::Class::Type sigils[] = {
    Ttx::Lexical::Class::Type::Expose,
  };
  Tetrodotoxin::Defines definitions(sigils, parts);

  EXPECT(alias.is<Tetrodotoxin::Dialect>());
  EXPECT(group.is<Tetrodotoxin::Dialect>());
  EXPECT(definitions.is<Tetrodotoxin::Dialect>());
  EXPECT(&definitions.resolve_context("alias"_view) == &alias);
  EXPECT(&definitions.resolve_context("group"_view) == &group);
}

PERIMORTEM_UNIT_TEST(DialectTests, package_alias) {
  Allocator::Arena arena;
  Types::Unsigned_8 value;
  Types::Unsigned_8 child;
  Alias value_import("Value"_view, value);
  Alias child_import("Child"_view, child);
  PackageValue item("item"_view, value);
  const Reference<Abstract> imported[] = {value_import, child_import, item};
  Group types("Types"_view, imported);
  const Reference<Abstract> imports[] = {types};
  Group source("Validation.Package"_view, imports);
  Tetrodotoxin::Dialects::Package package;
  Ttx::Lexical::Tokenizer tokenizer(
      arena,
      "// public package\n"
      "expose Value : alias = Types::Value;\n"
      "expose Copy : alias = Value;\n"
      "expose item : alias = Types::item;\n"
      "// nested exports\n"
      "expose Nested : group {\n"
      "  // nested child\n"
      "  expose Child : alias = Types::Child;\n"
      "  expose Parent : alias = Value;\n"
      "}"_view,
      "package.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, arena);

  const Abstract& evaluated = package.evaluate(cursor, source);

  ASSERT(evaluated.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(evaluated.is<Type>());
  EXPECT_TEXT(evaluated.get_name(), "Validation.Package"_view);
  EXPECT_EQ(
      evaluated.as<Tetrodotoxin::Model::Source>()
          .get_definitions()
          .get_abstracts()
          .get_size(),
      Count(4));
  const Abstract& exported = evaluated.resolve_context("Value"_view);
  ASSERT(exported.is<Alias>());
  EXPECT(&exported.resolve() == &value);
  EXPECT_EQ(exported.as<Alias>().get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      exported.as<Alias>().get_documentation().get_line(0),
      "public package"_view);
  EXPECT_TEXT(
      exported.as<Alias>().get_documentation().get_line(1),
      value.get_documentation().get_line(0));
  const Abstract& copy = evaluated.resolve_context("Copy"_view);
  ASSERT(copy.is<Alias>());
  EXPECT(&copy.resolve() == &value);
  const Abstract& addressable = evaluated.resolve_context("item"_view);
  ASSERT(addressable.is<Alias>());
  EXPECT(&addressable.resolve() == &value);
  const Abstract& nested = evaluated.resolve_context("Nested"_view);
  EXPECT_NOT(nested.is<Type>());
  EXPECT_TEXT(
      nested.as<Group>().get_documentation().get_line(0),
      "nested exports"_view);
  const Abstract& nested_child = nested.resolve_context("Child"_view);
  ASSERT(nested_child.is<Alias>());
  EXPECT(&nested_child.resolve() == &child);
  EXPECT_TEXT(
      nested_child.as<Alias>().get_documentation().get_line(0),
      "nested child"_view);
  const Abstract& parent = nested.resolve_context("Parent"_view);
  ASSERT(parent.is<Alias>());
  EXPECT(&parent.resolve() == &value);
  EXPECT_NOT(cursor.get_errors().has_errors());
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_sigil) {
  Allocator::Arena arena;
  Group source("Validation.Package"_view, {});
  Tetrodotoxin::Dialects::Package package;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "private Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, arena);

  const Abstract& evaluated = package.evaluate(cursor, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT(cursor.get_errors().has_errors());
  EXPECT_TEXT(
      cursor.get_errors().get_view()[0].get_message(),
      "Definition sigil is not accepted by this Dialect."_view);
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_dialect) {
  Allocator::Arena arena;
  Group source("Validation.Package"_view, {});
  Tetrodotoxin::Dialects::Package package;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "expose Value : missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, arena);

  const Abstract& evaluated = package.evaluate(cursor, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT(cursor.get_errors().has_errors());
  EXPECT_TEXT(
      cursor.get_errors().get_view()[0].get_message(),
      "Definition Dialect could not be selected."_view);
}

PERIMORTEM_UNIT_TEST(DialectTests, invalid_target) {
  Allocator::Arena arena;
  Group source("Validation.Package"_view, {});
  Tetrodotoxin::Dialects::Package package;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "expose Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, arena);

  const Abstract& evaluated = package.evaluate(cursor, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT(cursor.get_errors().has_errors());
  EXPECT_TEXT(
      cursor.get_errors().get_view()[0].get_message(),
      "Alias target could not be resolved."_view);
}
