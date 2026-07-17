// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/group.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/comments.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/scope.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

/// A Group leaf proves that containers preserve real child identities.
class GroupLeaf final : public Abstract {
 public:
  GroupLeaf(View::Bytes name) : name(name) {}

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

static Harness TtxGroup = {
  .name = "TTX::Group"_view,
};

PERIMORTEM_UNIT_TEST(TtxGroup, group_lookup) {
  static constexpr Static::Vector<View::Bytes, 1> lines = {{
    "A durable named collection."_view,
  }};
  Comments documentation(lines);
  GroupLeaf first("First"_view);
  GroupLeaf second("Second"_view);
  const Reference<Abstract> children[] = {first, second};
  Group group("Values"_view, children, documentation);

  EXPECT(group.is<Group>());
  EXPECT(&group.resolve_context("First"_view) == &first);
  EXPECT(&group.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT_EQ(group.get_abstracts().get_size(), Count(2));
  EXPECT_TEXT(group.get_documentation().get_line(0), lines[0]);
}

PERIMORTEM_UNIT_TEST(TtxGroup, scope_fallback) {
  GroupLeaf local_value("Value"_view);
  GroupLeaf outer_value("Value"_view);
  GroupLeaf imported("Imported"_view);
  const Reference<Abstract> local_children[] = {local_value};
  const Reference<Abstract> outer_children[] = {outer_value, imported};
  Group local("Local"_view, local_children);
  Group outer("Outer"_view, outer_children);
  Scope scope(local, outer);

  EXPECT(scope.is<Scope>());
  EXPECT_TEXT(scope.get_name(), "Local"_view);
  EXPECT(&scope.get_documentation() == &local.get_documentation());
  EXPECT(&scope.resolve_context("Value"_view) == &local_value);
  EXPECT(&scope.resolve_context("Imported"_view) == &imported);
  EXPECT(&scope.resolve_context("Missing"_view) == &Invalid::get_invalid());
}
