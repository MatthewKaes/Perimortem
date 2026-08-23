// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness RenderDialect = {
  .name = "Tetrodotoxin::Render::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(RenderDialect, retained_root) {
  static constexpr View::Bytes source = "//\ndialect : Render;"_view;
  Environment::Toolchain toolchain;
  auto dialect = toolchain.install<Render::Dialect>("Render"_view);
  ASSERT(dialect);
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Format"_view, "format.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Render::Language::Monograph>());
  const auto& monograph =
      static_cast<const Render::Language::Monograph&>(*interpreted);
  ASSERT(monograph.get_layer(*dialect));
  EXPECT(&*monograph.get_layer(*dialect) == &monograph);
  EXPECT(&workspace.resolve_context("Format"_view) == &monograph);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RenderDialect, progressive_declaration) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Render;\n"
      "public stage Fragment[] -> [];"_view;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Render::Dialect>("Render"_view));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Format"_view, "progressive-render.ttx"_view, source);

  EXPECT_NOT(interpreted);
  auto retained = workspace.get_monograph("progressive-render.ttx"_view);
  ASSERT(retained && retained->is<Render::Language::Monograph>());
  EXPECT(&workspace.resolve_context("Format"_view) == &*retained);
  EXPECT_EQ(errors.get_size(), Count(1));
}
