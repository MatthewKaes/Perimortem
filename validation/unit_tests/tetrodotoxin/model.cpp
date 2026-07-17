// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/alias.hpp"
#include "ttx/concept/comments.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Validation;

/// A named leaf keeps Source tests focused on containment rather than Type.
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
  SourceLeaf value("Value"_view);
  Alias alias("PublicValue"_view, value);
  const Reference<Abstract> definitions[] = {alias};
  const View::Bytes lines[] = {"public source"_view};
  Comments documentation(lines);
  Tetrodotoxin::Model::Source source(
      "Validation.Source"_view, definitions, documentation);

  EXPECT(source.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(source.is<Ttx::Model::Type>());
  EXPECT_TEXT(source.get_name(), "Validation.Source"_view);
  EXPECT_EQ(source.get_definitions().get_abstracts().get_size(), Count(1));
  EXPECT_TEXT(source.get_documentation().get_line(0), "public source"_view);
  EXPECT(&source.resolve_context("PublicValue"_view) == &alias);
  EXPECT(&source.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(&alias.resolve() == &value);
}
