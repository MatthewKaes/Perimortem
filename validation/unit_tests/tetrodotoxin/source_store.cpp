// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/resolution/source_store.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Resolution;
using namespace Validation;

static Harness TtxSourceStore = {
  .name = "Ttx::SourceStore"_view,
};

PERIMORTEM_UNIT_TEST(TtxSourceStore, touch) {
  SourceStore store;
  SourceStore::SourceRecord* root = store.require("root.ttx"_view);
  SourceStore::SourceRecord* library = store.require("library.ttx"_view);
  SourceStore::SourceRecord* shader = store.require("shader.ttx"_view);

  SourceStore::Dependency* edge = root->depend_on(*library);
  EXPECT(root->depend_on(*library) == edge);
  root->depend_on(*shader);

  EXPECT_EQ(root->get_dependencies().get_size(), Count(2));
  EXPECT_EQ(library->get_dependents().get_size(), Count(1));

  library->replace("dialect : Library;\n"_view);

  EXPECT_EQ(library->get_revision(), Count(1));
  EXPECT_EQ(root->get_revision(), Count(1));
  EXPECT_EQ(shader->get_revision(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxSourceStore, detach) {
  SourceStore store;
  SourceStore::SourceRecord* root = store.require("root.ttx"_view);
  SourceStore::SourceRecord* library = store.require("library.ttx"_view);
  SourceStore::SourceRecord* shader = store.require("shader.ttx"_view);

  root->depend_on(*library);
  root->depend_on(*shader);

  root->detach_dependencies();
  EXPECT(root->get_dependencies().is_empty());
  EXPECT(library->get_dependents().is_empty());
  EXPECT(shader->get_dependents().is_empty());

  library->touch();
  EXPECT_EQ(root->get_revision(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxSourceStore, cycle_touch) {
  SourceStore store;
  SourceStore::SourceRecord* first = store.require("first.ttx"_view);
  SourceStore::SourceRecord* second = store.require("second.ttx"_view);

  first->depend_on(*second);
  second->depend_on(*first);

  first->touch();
  EXPECT_EQ(first->get_revision(), Count(1));
  EXPECT_EQ(second->get_revision(), Count(1));

  first->detach();
  EXPECT(first->get_dependencies().is_empty());
  EXPECT(first->get_dependents().is_empty());
  EXPECT(second->get_dependencies().is_empty());
  EXPECT(second->get_dependents().is_empty());
}
