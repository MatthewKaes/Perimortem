// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Resolution;
using namespace Validation;

static constexpr View::Bytes disk_root =
    ".bin/bin/validation/ttx_res_disk_root.ttx"_view;
static constexpr View::Bytes disk_dep1 =
    ".bin/bin/validation/ttx_res_disk_dep1.ttx"_view;
static constexpr View::Bytes disk_dep2 =
    ".bin/bin/validation/ttx_res_disk_dep2.ttx"_view;
static constexpr View::Bytes disk_dep3 =
    ".bin/bin/validation/ttx_res_disk_dep3.ttx"_view;
static constexpr View::Bytes cycle_root =
    ".bin/bin/validation/ttx_res_cycle_root.ttx"_view;
static constexpr View::Bytes cycle_dep1 =
    ".bin/bin/validation/ttx_res_cycle_dep1.ttx"_view;
static constexpr View::Bytes cycle_dep2 =
    ".bin/bin/validation/ttx_res_cycle_dep2.ttx"_view;
static constexpr View::Bytes cycle_dep3 =
    ".bin/bin/validation/ttx_res_cycle_dep3.ttx"_view;

static auto remove_disk_sources() -> void {
  File::remove(disk_root);
  File::remove(disk_dep1);
  File::remove(disk_dep2);
  File::remove(disk_dep3);
  File::remove(cycle_root);
  File::remove(cycle_dep1);
  File::remove(cycle_dep2);
  File::remove(cycle_dep3);
}

static Harness TtxResolution = {
  .name = "Ttx::Resolution"_view,
  .setup = []() { remove_disk_sources(); },
  .teardown = []() { remove_disk_sources(); },
};

static constexpr View::Bytes library_source = "dialect : Library;\n"_view;
static constexpr View::Bytes memory_a_source =
    "dialect : Library;\n"
    "import B : Library = (.source = \"b.ttx\");\n"_view;
static constexpr View::Bytes memory_b_cycle_source =
    "dialect : Library;\n"
    "import A : Library = (.source = \"a.ttx\");\n"_view;
static constexpr View::Bytes memory_c_source =
    "dialect : Library;\n"
    "import A : Library = (.source = \"a.ttx\");\n"_view;

static auto write_source(View::Bytes source_path, View::Bytes source) -> Bool {
  File file;
  file.update_contents(source);
  return file.write(source_path);
}

static auto error_path(const Resolver::Context& context, Count index = 0)
    -> View::Bytes {
  return context.get_errors()[index].get_source_path();
}

static auto error_message(const Resolver::Context& context, Count index = 0)
    -> View::Bytes {
  return context.get_errors()[index].get_message();
}

static auto first_error_is(
    const Resolver::Context& context,
    View::Bytes source_path,
    View::Bytes message) -> Bool {
  return context.get_errors().get_size() == 1 &&
         error_path(context) == source_path &&
         error_message(context) == message;
}

static auto has_error(
    const Resolver::Context& context,
    View::Bytes source_path,
    View::Bytes message) -> Bool {
  for (Count error_index = 0; error_index < context.get_errors().get_size();
       error_index++) {
    if (error_path(context, error_index) == source_path &&
        error_message(context, error_index) == message) {
      return True;
    }
  }

  return False;
}

PERIMORTEM_UNIT_TEST(TtxResolution, package_imports) {
  Resolver resolver;
  Resolver::Context context;

  const Ttx::Source* root_source = resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = TTX::Graphics;\n"_view);
  ASSERT(root_source != nullptr);
  EXPECT_NOT(context.has_errors());
  EXPECT_EQ(root_source->get_imports().get_size(), Count(1));
  context.reset();

  const Ttx::Source* package_source = resolver.resolve("TTX::Graphics"_view);
  ASSERT(package_source != nullptr);
  EXPECT_EQ(package_source->get_imports().get_size(), Count(4));

  EXPECT(
      resolver.resolve(root_source->get_imports()[0].get_source_name()) ==
      package_source);
  EXPECT_NOT(resolver.resolve("TTX.Graphics"_view));
  EXPECT_NOT(resolver.resolve(
      "tetrodotoxin/packages/graphics/shaders/default_2d.ttx"_view));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, missing_imports) {
  Resolver resolver;
  Resolver::Context context;
  EXPECT_NOT(resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import MissingA : Library = (.source = \"missing_a.ttx\");\n"
      "import MissingB : Library = (.source = \"missing_b.ttx\");\n"_view));

  ASSERT_EQ(context.get_errors().get_size(), Count(2));
  EXPECT(has_error(
      context, "unit/missing_a.ttx"_view,
      "Imported source file could not be read."_view));
  EXPECT(has_error(
      context, "unit/missing_b.ttx"_view,
      "Imported source file could not be read."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, parse_error_retry) {
  Resolver resolver;
  Resolver::Context context;
  EXPECT_NOT(resolver.load_source(
      context, "unit/root.ttx"_view, "dialect : ;\n"_view));
  EXPECT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(context.has_errors());

  ASSERT(resolver.load_source(context, "unit/root.ttx"_view, library_source));
  context.reset();
  EXPECT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(context.has_errors());

  const Ttx::Source* source =
      resolver.load_source(context, "unit/memory.ttx"_view, library_source);
  ASSERT(source != nullptr);
  EXPECT_TEXT(source->get_source(), library_source);
}

PERIMORTEM_UNIT_TEST(TtxResolution, dialect_mismatch) {
  Resolver resolver;
  Resolver::Context context;
  ASSERT(resolver.load_source(context, "unit/target.ttx"_view, library_source));
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Target : Shader = (.source = \"target.ttx\");\n"_view));

  EXPECT(first_error_is(
      context, "unit/root.ttx"_view,
      "Imported source dialect does not match the requested dialect."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT(resolver.resolve("unit/target.ttx"_view));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, memory_only_cache) {
  Resolver resolver;
  Resolver::Context context;
  // Fails to load since it can't find "b.ttx" on disk and it's not in memory.
  // In memory files are immune to cycle creation since they must be loaded in
  // order.
  EXPECT_NOT(resolver.load_source(context, "unit/a.ttx"_view, memory_a_source));
  EXPECT(has_error(
      context, "unit/b.ttx"_view,
      "Imported source file could not be read."_view));
  context.reset();

  // Fails to load since it can't find "a.ttx" on disk and it's also not in
  // memory since it failed to load.
  EXPECT_NOT(
      resolver.load_source(context, "unit/b.ttx"_view, memory_b_cycle_source));
  EXPECT(has_error(
      context, "unit/a.ttx"_view,
      "Imported source file could not be read."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT_NOT(resolver.resolve("unit/b.ttx"_view));

  // Load a valid B into memory.
  ASSERT(resolver.load_source(context, "unit/b.ttx"_view, library_source));
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));

  // A can now load into memory since we have a valid B.
  const Ttx::Source* a =
      resolver.load_source(context, "unit/a.ttx"_view, memory_a_source);
  ASSERT(a != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve("unit/a.ttx"_view) == a);
  EXPECT(resolver.resolve("unit/b.ttx"_view));
  EXPECT(
      resolver.resolve(a->get_imports()[0].get_source_name()) ==
      resolver.resolve("unit/b.ttx"_view));

  // Load a third library that depends on B transitively through A.
  const Ttx::Source* c =
      resolver.load_source(context, "unit/c.ttx"_view, memory_c_source);
  ASSERT(c != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));
  EXPECT(resolver.resolve("unit/c.ttx"_view) == c);
  EXPECT(
      resolver.resolve(c->get_imports()[0].get_source_name()) ==
      resolver.resolve("unit/a.ttx"_view));

  // Update B to be a shader. Since A depends on B it will need to reevaluate
  // if it's still valid. B is still a valid source, but A and C are removed.
  const Ttx::Source* shader_b = resolver.load_source(
      context, "unit/b.ttx"_view, "dialect : Shader;\n"_view);
  ASSERT(shader_b != nullptr);
  EXPECT(has_error(
      context, "unit/a.ttx"_view,
      "Imported source dialect does not match the requested dialect."_view));
  EXPECT(has_error(
      context, "unit/c.ttx"_view,
      "Couldn't resolve import `unit/a.ttx`."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view) == shader_b);
  EXPECT_NOT(resolver.resolve("unit/c.ttx"_view));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, break_cached) {
  Resolver resolver;
  Resolver::Context context;
  // Fails to load since it can't find "b.ttx" on disk and it's not in memory.
  // In memory files are immune to cycle creation since they must be loaded in
  // order.
  EXPECT_NOT(resolver.load_source(context, "unit/a.ttx"_view, memory_a_source));
  EXPECT(has_error(
      context, "unit/b.ttx"_view,
      "Imported source file could not be read."_view));
  context.reset();

  // Fails to load since it can't find "a.ttx" on disk and it's also not in
  // memory since it failed to load.
  EXPECT_NOT(
      resolver.load_source(context, "unit/b.ttx"_view, memory_b_cycle_source));
  EXPECT(has_error(
      context, "unit/a.ttx"_view,
      "Imported source file could not be read."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT_NOT(resolver.resolve("unit/b.ttx"_view));

  // Load a valid B into memory.
  ASSERT(resolver.load_source(context, "unit/b.ttx"_view, library_source));
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));

  // A can now load into memory since we have a valid B.
  const Ttx::Source* a =
      resolver.load_source(context, "unit/a.ttx"_view, memory_a_source);
  ASSERT(a != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve("unit/a.ttx"_view) == a);
  EXPECT(resolver.resolve("unit/b.ttx"_view));

  // Load a third library that depends on B transitively through A.
  const Ttx::Source* c =
      resolver.load_source(context, "unit/c.ttx"_view, memory_c_source);
  ASSERT(c != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));
  EXPECT(resolver.resolve("unit/c.ttx"_view) == c);
  EXPECT(
      resolver.resolve(c->get_imports()[0].get_source_name()) ==
      resolver.resolve("unit/a.ttx"_view));

  // A broken producer is removed, and every cached consumer that depends on it
  // is removed with it.
  EXPECT_NOT(resolver.load_source(
      context, "unit/b.ttx"_view, "bad ttx contents"_view));
  EXPECT(context.has_errors());

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT_NOT(resolver.resolve("unit/b.ttx"_view));
  EXPECT_NOT(resolver.resolve("unit/c.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TtxResolution, disk_chain) {
  Resolver resolver;
  Resolver::Context context;

  ASSERT(write_source(
      disk_root,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_disk_dep1.ttx\");\n"
      "import Dep2 : Library = (.source = \"ttx_res_disk_dep2.ttx\");\n"_view));
  ASSERT(write_source(disk_dep1, library_source));
  ASSERT(write_source(
      disk_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_disk_dep1.ttx\");\n"_view));

  const Ttx::Source* root = resolver.load_source(context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve(disk_root) == root);
  const Ttx::Source* dep1 = resolver.resolve(disk_dep1);
  const Ttx::Source* dep2 = resolver.resolve(disk_dep2);
  ASSERT(dep1 != nullptr);
  ASSERT(dep2 != nullptr);
  EXPECT_EQ(root->get_imports().get_size(), Count(2));
  EXPECT(resolver.resolve(root->get_imports()[0].get_source_name()) == dep1);
  EXPECT(resolver.resolve(root->get_imports()[1].get_source_name()) == dep2);
  EXPECT_EQ(dep2->get_imports().get_size(), Count(1));
  EXPECT(resolver.resolve(dep2->get_imports()[0].get_source_name()) == dep1);
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, disk_cycle) {
  Resolver resolver;
  Resolver::Context context;

  ASSERT(write_source(
      cycle_root,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_cycle_dep1.ttx\");\n"
      "import Dep2 : Library = (.source = \"ttx_res_cycle_dep2.ttx\");\n"
      "import Dep3 : Library = (.source = \"ttx_res_cycle_dep3.ttx\");\n"_view));
  ASSERT(write_source(
      cycle_dep1,
      "dialect : Library;\n"
      "import Dep2 : Library = (.source = \"ttx_res_cycle_dep2.ttx\");\n"_view));
  ASSERT(write_source(
      cycle_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_cycle_dep1.ttx\");\n"_view));
  ASSERT(write_source(cycle_dep3, library_source));

  EXPECT_NOT(resolver.load_source(context, cycle_root));
  EXPECT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve(cycle_root));
  EXPECT_NOT(resolver.resolve(cycle_dep1));
  EXPECT_NOT(resolver.resolve(cycle_dep2));
  EXPECT(resolver.resolve(cycle_dep3));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxResolution, bad_update) {
  Resolver resolver;
  Resolver::Context context;

  ASSERT(write_source(
      disk_root,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_disk_dep1.ttx\");\n"
      "import Dep2 : Library = (.source = \"ttx_res_disk_dep2.ttx\");\n"
      "import Dep3 : Library = (.source = \"ttx_res_disk_dep3.ttx\");\n"_view));
  ASSERT(write_source(disk_dep1, library_source));
  ASSERT(write_source(
      disk_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = (.source = \"ttx_res_disk_dep1.ttx\");\n"_view));
  ASSERT(write_source(disk_dep3, library_source));

  Ttx::Source* root = resolver.load_source(context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve(disk_root) == root);
  EXPECT(resolver.resolve(disk_dep1));
  EXPECT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));
  Ttx::Source* dep1 = resolver.resolve(disk_dep1);
  Ttx::Source* dep2 = resolver.resolve(disk_dep2);
  ASSERT(dep1 != nullptr);
  ASSERT(dep2 != nullptr);

  EXPECT_EQ(root->get_imports().get_size(), Count(3));
  EXPECT(resolver.resolve(root->get_imports()[0].get_source_name()) == dep1);
  EXPECT(resolver.resolve(root->get_imports()[1].get_source_name()) == dep2);
  EXPECT(
      resolver.resolve(root->get_imports()[2].get_source_name()) ==
      resolver.resolve(disk_dep3));

  EXPECT_NOT(resolver.load_source(context, disk_dep1, "bad ttx contents"_view));
  EXPECT(context.has_errors());
  context.reset();

  // Messing up dep1 has splash damage that invalidates the root and dep2,
  // however dep3 survives.
  EXPECT_NOT(resolver.resolve(disk_root));
  EXPECT_NOT(resolver.resolve(disk_dep1));
  EXPECT_NOT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));

  // Reloading root from disk will keep dep3 around since it was already loaded,
  // however since dep1 and dep2 were previously unloaded they will be reloaded
  // from disk.
  root = resolver.load_source(context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve(disk_root) == root);
  EXPECT(resolver.resolve(disk_dep1));
  EXPECT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));

  // Since dep1 and dep2 were reloaded, their old pointers should not point to
  // the new Source objects.

  // Import matches graph address.
  EXPECT(
      resolver.resolve(root->get_imports()[1].get_source_name()) ==
      resolver.resolve(disk_dep2));
  // Old source address doesn't point to the new source address.
  EXPECT_NOT(dep1 == resolver.resolve(disk_dep1));
  EXPECT_NOT(dep2 == resolver.resolve(disk_dep2));
}

PERIMORTEM_UNIT_TEST(TtxResolution, package_loading) {
  Resolver resolver;
  Resolver::Context context;
  const Ttx::Source* root = resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = TTX::Graphics;\n"
      "@private Default2D : Alias = Graphics::Shaders::Default2D;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  // TTX::Graphics import loads tetrodotoxin/packages/graphics/package.ttx.
  const Ttx::Source* graphics = resolver.resolve("TTX::Graphics"_view);
  ASSERT(graphics != nullptr);
  EXPECT(
      resolver.resolve(root->get_imports()[0].get_source_name()) == graphics);
  EXPECT_NOT(resolver.resolve(
      "tetrodotoxin/packages/graphics/shaders/default_2d.ttx"_view));

  const Ttx::Source* source = resolver.resolve("unit/root.ttx"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(context.has_errors());
  EXPECT_EQ(source->get_imports().get_size(), Count(1));
  EXPECT_TEXT(
      source->get_body_text(),
      "@private Default2D : Alias = Graphics::Shaders::Default2D;\n"_view);

  EXPECT_NOT(resolver.load_source(
      context, "unit/direct.ttx"_view,
      "dialect : Library;\n"
      "import Default2D : Shader = "
      "(.source = "
      "\"../tetrodotoxin/packages/graphics/shaders/default_2d.ttx\");"
      "\n"_view));
  EXPECT(has_error(
      context, "unit/direct.ttx"_view,
      "Package private source cannot be imported directly."_view));
  EXPECT_NOT(resolver.resolve("unit/direct.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TtxResolution, package_chain) {
  Resolver resolver;
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "packages/user/ui/package.ttx"_view,
      "dialect : Package;\n"
      "import Core : Package = User::Core;\n"
      "@package_name = User::Ui;\n"
      "@public Core : Package = Core;\n"_view));
  EXPECT(has_error(
      context, "packages/user/ui/package.ttx"_view,
      "Import could not find valid `user/core/package.ttx` for package "
      "User::Core.\nMake sure the Package is at the expected location or a "
      "Package that declares `@package_name = User::Core` is explicitly "
      "loaded."_view));
  context.reset();

  const Ttx::Source* core = resolver.load_source(
      context, "packages/user/core/package.ttx"_view,
      "dialect : Package;\n"
      "@package_name = User::Core;\n"
      "@public Value : alias = Internal::Value;\n"_view);
  ASSERT(core != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Ttx::Source* ui = resolver.load_source(
      context, "packages/user/ui/package.ttx"_view,
      "dialect : Package;\n"
      "import Core : Package = User::Core;\n"
      "@package_name = User::Ui;\n"
      "@public Core : Package = Core;\n"_view);
  ASSERT(ui != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Ttx::Source* root = resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Ui : Package = User::Ui;\n"
      "@private Value : Alias = Ui::Core::Value;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT(resolver.resolve("unit/root.ttx"_view) == root);
  EXPECT(resolver.resolve("User::Ui"_view) == ui);
  EXPECT(resolver.resolve("User::Core"_view) == core);
  EXPECT(resolver.resolve(root->get_imports()[0].get_source_name()) == ui);
  EXPECT(resolver.resolve(ui->get_imports()[0].get_source_name()) == core);

  EXPECT_NOT(resolver.load_source(
      context, "packages/user/core/package.ttx"_view, "bad ttx contents"_view));
  EXPECT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(resolver.resolve("User::Ui"_view));
  EXPECT_NOT(resolver.resolve("User::Core"_view));
}

PERIMORTEM_UNIT_TEST(TtxResolution, bad_package) {
  Resolver resolver;
  Resolver::Context context;
  EXPECT_NOT(resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = User::Package::Test;\n"_view));
  EXPECT(has_error(
      context, "unit/root.ttx"_view,
      "Import could not find valid `user/package/test/package.ttx` for package "
      "User::Package::Test.\nMake sure the Package is at the expected location "
      "or a Package that declares `@package_name = User::Package::Test` is "
      "explicitly loaded."_view));
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(resolver.resolve("User::Package::Test"_view));

  // The Package dialect publishes @package_name as the resolver cache key.
  const Ttx::Source* user_package = resolver.load_source(
      context, "arbitrary/path/package.ttx"_view,
      "dialect : Package;\n"
      "@package_name = User::Package::Test;\n"_view);
  ASSERT(user_package != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT(resolver.resolve("User::Package::Test"_view));

  // Now that we have the package in memory.
  const Ttx::Source* source = resolver.load_source(
      context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = User::Package::Test;\n"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(context.has_errors());

  EXPECT(resolver.resolve("unit/root.ttx"_view));
  EXPECT(resolver.resolve("User::Package::Test"_view));

  // Root should have the correct import which we can now resolve.
  // Loading root with the cached package should also not invalidate
  // user_package so it's address should still be valid.
  EXPECT(
      user_package ==
      resolver.resolve(source->get_imports()[0].get_source_name()));
}
