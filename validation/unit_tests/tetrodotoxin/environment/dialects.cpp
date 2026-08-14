// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class TestDialect : public Language::Dialect {
 public:
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

class IndependentDialect : public TestDialect {};

class LowerDialect : public TestDialect {};

class MiddleDialect : public TestDialect {
 public:
  explicit MiddleDialect(LowerDialect& lower) : lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class LeftDialect : public TestDialect {
 public:
  explicit LeftDialect(LowerDialect& lower) : lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class RightDialect : public TestDialect {
 public:
  explicit RightDialect(LowerDialect& lower) : lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class TopDialect : public TestDialect {
 public:
  TopDialect(LeftDialect& left, RightDialect& right)
      : left(left), right(right) {}

  auto get_left() const -> const LeftDialect& { return left; }
  auto get_right() const -> const RightDialect& { return right; }

 private:
  LeftDialect& left;
  RightDialect& right;
};

class RootDialect : public TestDialect {
 public:
  RootDialect() = default;
  explicit RootDialect(Language::Dialect&) {}
};

class ChildDialect : public TestDialect {
 public:
  explicit ChildDialect(RootDialect& root) : root(root) {}

  auto get_root() const -> const RootDialect& { return root; }

 private:
  RootDialect& root;
};

static Harness EnvironmentDialects = {
  .name = "Tetrodotoxin::Environment::Dialects"_view,
};

PERIMORTEM_UNIT_TEST(EnvironmentDialects, dependency_free_and_chain) {
  Environment::Workspace workspace;

  auto independent =
      workspace.install_dialect<IndependentDialect>("Independent"_view);
  auto lower = workspace.install_dialect<LowerDialect>("Lower"_view);
  ASSERT(independent);
  ASSERT(lower);
  auto middle = workspace.install_dialect<MiddleDialect>("Middle"_view, *lower);

  ASSERT(middle);
  EXPECT(&middle->get_lower() == lower);
}

PERIMORTEM_UNIT_TEST(EnvironmentDialects, shared_diamond) {
  Environment::Workspace workspace;

  auto lower = workspace.install_dialect<LowerDialect>("Lower"_view);
  ASSERT(lower);
  auto left = workspace.install_dialect<LeftDialect>("Left"_view, *lower);
  auto right = workspace.install_dialect<RightDialect>("Right"_view, *lower);
  ASSERT(left);
  ASSERT(right);
  auto top = workspace.install_dialect<TopDialect>("Top"_view, *left, *right);

  ASSERT(top);
  EXPECT(&top->get_left() == left);
  EXPECT(&top->get_right() == right);
  EXPECT(&top->get_left().get_lower() == lower);
  EXPECT(&top->get_right().get_lower() == lower);
}

PERIMORTEM_UNIT_TEST(EnvironmentDialects, rejects_unowned_dependencies) {
  Environment::Workspace local;
  Environment::Workspace foreign;
  RootDialect missing;

  auto missing_result =
      local.install_dialect<ChildDialect>("Missing"_view, missing);
  auto foreign_root = foreign.install_dialect<RootDialect>("ForeignRoot"_view);
  ASSERT(foreign_root);
  auto foreign_result =
      local.install_dialect<ChildDialect>("Foreign"_view, *foreign_root);

  EXPECT_NOT(missing_result);
  EXPECT_NOT(foreign_result);
}

PERIMORTEM_UNIT_TEST(EnvironmentDialects, rejects_duplicates_and_cycle) {
  Environment::Workspace workspace;

  auto root = workspace.install_dialect<RootDialect>("Root"_view);
  ASSERT(root);
  auto child = workspace.install_dialect<ChildDialect>("Child"_view, *root);
  ASSERT(child);

  EXPECT_NOT(workspace.install_dialect<IndependentDialect>("Root"_view));
  EXPECT_NOT(workspace.install_dialect<RootDialect>("SecondRoot"_view));
  EXPECT_NOT(workspace.install_dialect<RootDialect>("CycleRoot"_view, *child));
  EXPECT(&child->get_root() == root);
}
