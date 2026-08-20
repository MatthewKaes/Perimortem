// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/toolchain.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class TestDialect : public Language::Dialect {
 public:
  TestDialect(View::Bytes name) : Language::Dialect(name) {}

  auto interpret(Cursor&, const Documentation&, const Anchor&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }
};

class IndependentDialect : public TestDialect {
 public:
  using TestDialect::TestDialect;
};

class LowerDialect : public TestDialect {
 public:
  using TestDialect::TestDialect;
};

class MiddleDialect : public TestDialect {
 public:
  MiddleDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class LeftDialect : public TestDialect {
 public:
  LeftDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class RightDialect : public TestDialect {
 public:
  RightDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class TopDialect : public TestDialect {
 public:
  TopDialect(View::Bytes name, LeftDialect& left, RightDialect& right)
      : TestDialect(name), left(left), right(right) {}

  auto get_left() const -> const LeftDialect& { return left; }
  auto get_right() const -> const RightDialect& { return right; }

 private:
  LeftDialect& left;
  RightDialect& right;
};

class RootDialect : public TestDialect {
 public:
  RootDialect(View::Bytes name) : TestDialect(name) {}

  RootDialect(View::Bytes name, Language::Dialect&) : TestDialect(name) {}
};

class ChildDialect : public TestDialect {
 public:
  ChildDialect(View::Bytes name, RootDialect& root)
      : TestDialect(name), root(root) {}

  auto get_root() const -> const RootDialect& { return root; }

 private:
  RootDialect& root;
};

static Harness EnvironmentToolchain = {
  .name = "Tetrodotoxin::Environment::Toolchain"_view,
};

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, dependency_free_and_chain) {
  Environment::Toolchain toolchain;

  auto independent = toolchain.install<IndependentDialect>("Independent"_view);
  auto lower = toolchain.install<LowerDialect>("Lower"_view);
  ASSERT(independent);
  ASSERT(lower);
  auto middle = toolchain.install<MiddleDialect>("Middle"_view, *lower);

  ASSERT(middle);
  EXPECT(&middle->get_lower() == &*lower);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, shared_diamond) {
  Environment::Toolchain toolchain;

  auto lower = toolchain.install<LowerDialect>("Lower"_view);
  ASSERT(lower);
  auto left = toolchain.install<LeftDialect>("Left"_view, *lower);
  auto right = toolchain.install<RightDialect>("Right"_view, *lower);
  ASSERT(left);
  ASSERT(right);
  auto top = toolchain.install<TopDialect>("Top"_view, *left, *right);

  ASSERT(top);
  EXPECT(&top->get_left() == &*left);
  EXPECT(&top->get_right() == &*right);
  EXPECT(&top->get_left().get_lower() == &*lower);
  EXPECT(&top->get_right().get_lower() == &*lower);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, rejects_unowned_dependencies) {
  Environment::Toolchain local;
  Environment::Toolchain foreign;
  RootDialect missing("Missing"_view);

  auto missing_result = local.install<ChildDialect>("Missing"_view, missing);
  auto foreign_root = foreign.install<RootDialect>("ForeignRoot"_view);
  ASSERT(foreign_root);
  auto foreign_result =
      local.install<ChildDialect>("Foreign"_view, *foreign_root);

  EXPECT_NOT(missing_result);
  EXPECT_NOT(foreign_result);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, maps_names_to_instances) {
  Environment::Toolchain toolchain;

  auto root = toolchain.install<RootDialect>("Root"_view);
  ASSERT(root);
  auto child = toolchain.install<ChildDialect>("Child"_view, *root);
  ASSERT(child);

  EXPECT_NOT(toolchain.install<IndependentDialect>("Root"_view));
  EXPECT(toolchain.install<RootDialect>("SecondRoot"_view));
  EXPECT(toolchain.install<RootDialect>("ContextRoot"_view, *child));
  EXPECT(&child->get_root() == &*root);
}
