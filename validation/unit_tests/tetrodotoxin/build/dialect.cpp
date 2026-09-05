// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/build/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/build/monograph.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness BuildDialect = {
  .name = "Tetrodotoxin::Build::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(BuildDialect, retains_authored_infrastructure) {
  Environment::Toolchain toolchain;
  auto environment =
      toolchain.install<Environment::Dialect>("Environment"_view);
  ASSERT(environment);
  ASSERT(toolchain.install<Build::Dialect>("Build"_view, *environment));

  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Build"_view, "build.ttx"_view,
      "// Builds one application.\n"
      "dialect : Build;\n"
      "// Locates the package.\n"
      "private Product : alias = source(\"package.ttx\");\n"
      "environment = Environment {\n"
      "  .output_root = \"build\",\n"
      "  .reachable_roots = [\".\", sdk],\n"
      "  .repositories = [sdk_repository(\"standard\")],\n"
      "  .plugins = [sdk(\"Tetrodotoxin.GraphText\")],\n"
      "}\n"
      "// Emits Graph Text for one exported identity.\n"
      "public Graph : product = Product::App {\n"
      "  .terminal = sdk(\"Tetrodotoxin.GraphText\")::GraphText,\n"
      "};\n"_view);

  ASSERT(interpreted);
  auto build = interpreted->select<Build::Monograph>();
  ASSERT(build);
  EXPECT_TEXT(build->get_package_locator(), "package.ttx"_view);
  ASSERT_EQ(build->get_products().get_size(), Count(1));
  EXPECT_TEXT(build->get_products()[0].name, "Graph"_view);
  EXPECT_TEXT(build->get_products()[0].package_route, "App"_view);
  EXPECT_TEXT(
      build->get_products()[0].terminal_artifact,
      "Tetrodotoxin.GraphText"_view);
  EXPECT_TEXT(build->get_products()[0].terminal_export, "GraphText"_view);
  EXPECT(errors.is_empty());
}
