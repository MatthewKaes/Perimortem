// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/alias.hpp"
#include "ttx/concept/comments.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Validation;

/// A named leaf keeps Source tests focused on general Abstract resolution.
class SourceLeaf final : public Abstract {
 public:
  explicit SourceLeaf(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }

 private:
  View::Bytes name;
};

static Harness SourceTests = {
  .name = "Source"_view,
};

PERIMORTEM_UNIT_TEST(SourceTests, source_root) {
  Tetrodotoxin::Model::Source source(
      "expose PublicValue : alias = Value;"_view, "validation/source.ttx"_view);
  SourceLeaf& value = source.get_arena().construct<SourceLeaf>("Value"_view);
  const Static::Vector<View::Bytes, 1> lines = {{"public source"_view}};
  Comments& documentation =
      source.get_arena().construct<Comments>(lines.get_view());
  Alias& alias = source.get_arena().construct<Alias>(
      "PublicValue"_view, value, documentation);

  Bool rooted = source.root(alias);

  EXPECT(rooted);
  ASSERT(source.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(source.is<Ttx::Model::Type>());
  EXPECT(source.get_name().is_empty());
  EXPECT_TEXT(source.get_path(), "validation/source.ttx"_view);
  EXPECT_TEXT(source.get_text(), "expose PublicValue : alias = Value;"_view);
  EXPECT_TEXT(source.get_tokenizer().get_source_name(), source.get_path());
  EXPECT_EQ(source.get_definitions().get_size(), Count(1));
  EXPECT(source.get_documentation().is_empty());
  EXPECT(&source.resolve_context("PublicValue"_view) == &alias);
  EXPECT(&source.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(&alias.resolve() == &value);
}

PERIMORTEM_UNIT_TEST(SourceTests, duplicate_root) {
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
  SourceLeaf& first = source.get_arena().construct<SourceLeaf>("Value"_view);
  SourceLeaf& second = source.get_arena().construct<SourceLeaf>("Value"_view);

  Bool first_rooted = source.root(first);
  Bool second_rooted = source.root(second);

  EXPECT(first_rooted);
  EXPECT_NOT(second_rooted);
  EXPECT(source.get_path().is_empty());
  EXPECT(source.get_text().is_empty());
  EXPECT(&source.resolve_context("Value"_view) == &first);
}

PERIMORTEM_UNIT_TEST(SourceTests, anonymous_roots) {
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
  SourceLeaf& first = source.get_arena().construct<SourceLeaf>(View::Bytes{});
  SourceLeaf& second = source.get_arena().construct<SourceLeaf>(View::Bytes{});

  Bool first_rooted = source.root(first);
  Bool second_rooted = source.root(second);
  Bool first_repeated = source.root(first);

  EXPECT(first_rooted);
  EXPECT(second_rooted);
  EXPECT_NOT(first_repeated);
  EXPECT_EQ(source.get_definitions().get_size(), Count(2));
  EXPECT(source.resolve_context(View::Bytes{}).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(SourceTests, owns_input_and_tokenizer) {
  Dynamic::Bytes input("expose Value : alias = Target;"_view);
  Dynamic::Bytes path("snippet.ttx"_view);
  Tetrodotoxin::Model::Source source(input, path);

  input.clear();
  path.clear();

  EXPECT_TEXT(source.get_text(), "expose Value : alias = Target;"_view);
  EXPECT_TEXT(source.get_path(), "snippet.ttx"_view);
  EXPECT_TEXT(source.get_tokenizer().get_source_text(), source.get_text());
  EXPECT_TEXT(source.get_tokenizer().get_source_name(), source.get_path());
  EXPECT_NOT(source.get_tokenizer().is_empty());
}
