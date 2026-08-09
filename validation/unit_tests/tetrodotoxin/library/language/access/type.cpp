// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Validation;

static Harness TypeAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Type"_view,
};

class AccessLeaf : public Type {
 public:
  explicit AccessLeaf(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

class AccessContext : public Abstract {
 public:
  AccessContext(View::Bytes name, View::Bytes child_name, const Abstract& child)
      : name(name), child_name(child_name), child(child) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return route == child_name ? child : Invalid::get_invalid();
  }

 private:
  View::Bytes name;
  View::Bytes child_name;
  const Abstract& child;
};

static auto rejects_type_access(View::Bytes source) -> Bool {
  Allocator::Arena domain;
  Errors errors;
  Tokenizer tokenizer(domain, source, "type-access-rejection.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed = Library::Language::Access::Type::parse(cursor);
  return !parsed && !errors.is_empty();
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, progressive_context) {
  static_assert(!__is_base_of(Abstract, Library::Language::Access::Type));
  static constexpr View::Bytes source = "Root::Nested::Scalar"_view;
  Allocator::Arena domain;
  Errors errors;
  Tokenizer tokenizer(domain, source, "type-access.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto access = Library::Language::Access::Type::parse(cursor);
  ASSERT(access);
  ASSERT(cursor.matches(Code::Type::Terminal));

  AccessLeaf scalar("Scalar"_view);
  Alias scalar_target("target"_view, scalar);
  Alias scalar_alias("Scalar"_view, scalar_target);
  AccessContext nested("Nested"_view, "Scalar"_view, scalar_alias);
  AccessContext root("Root"_view, "Nested"_view, nested);
  Alias root_alias("Root"_view, root);
  AccessContext source_context("Source"_view, "Root"_view, root_alias);
  Alias source_alias("SourceAlias"_view, source_context);

  EXPECT_TEXT(access->get_route(), source);
  EXPECT_TEXT(access->get_root(), "Root"_view);
  EXPECT_TEXT(
      access->get_anchor().get_token().caculate_text(source), "Root"_view);
  EXPECT_TEXT(access->get_anchor().get_span().caculate_text(source), source);
  EXPECT(&access->resolve(source_context) == &scalar);
  EXPECT(&access->resolve(source_alias) == &scalar);
  EXPECT(&access->resolve_from(root) == &scalar);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, exact_segment_failure) {
  static constexpr View::Bytes source = "Root::Missing"_view;
  Allocator::Arena domain;
  Errors errors;
  Tokenizer tokenizer(domain, source, "missing-type-access.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto access = Library::Language::Access::Type::parse(cursor);
  ASSERT(access);

  AccessLeaf scalar("Scalar"_view);
  AccessContext root("Root"_view, "Scalar"_view, scalar);
  AccessContext source_context("Source"_view, "Root"_view, root);
  EXPECT(&access->resolve(source_context) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, grammar) {
  EXPECT(rejects_type_access("Root ::Nested"_view));
  EXPECT(rejects_type_access("Root:: Nested"_view));
  EXPECT(rejects_type_access("Root::"_view));
  EXPECT(rejects_type_access("Root::value"_view));
  EXPECT(rejects_type_access("value"_view));
}
