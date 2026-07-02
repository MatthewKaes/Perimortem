// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Resolution;
using namespace Validation;

static Harness TtxResolution = {
  .name = "Ttx::Resolution"_view,
};

static constexpr View::Bytes library_source = "dialect : Library;\n"_view;
static constexpr View::Bytes package_header = "dialect : Package;\n"_view;

static auto seed(Resolver& resolver, View::Bytes source_path, View::Bytes source_text)
    -> void {
  resolver.replace_source(source_path, source_text);
}

static auto error_path(const Resolver::Context& context, Count index = 0)
    -> View::Bytes { return context.get_errors()[index].get_source_path(); }

static auto error_message(const Resolver::Context& context, Count index = 0)
    -> View::Bytes { return context.get_errors()[index].get_message(); }

static auto first_error_is(
    const Resolver::Context& context,
    View::Bytes source_path,
    View::Bytes message) -> Bool {
  return context.get_errors().get_size() == 1 && error_path(context) == source_path &&
         error_message(context) == message;
}

PERIMORTEM_UNIT_TEST(TtxResolution, package_imports) {
  Resolver resolver;
  Resolver::Context context;

  seed(
      resolver,
      "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = TTX::Graphics;\n"_view);

  const Ttx::Source* root_source =
      resolver.resolve_path(context, "unit/root.ttx"_view);
  ASSERT(root_source != nullptr);
  EXPECT_NOT(context.has_errors());
  EXPECT_EQ(root_source->get_imports().get_size(), Count(1));

  const Ttx::Source* package_source =
      resolver.resolve_path(
          context, "tetrodotoxin/packages/graphics/package.ttx"_view);
  ASSERT(package_source != nullptr);
  EXPECT_EQ(package_source->get_imports().get_size(), Count(4));

  EXPECT(resolver.resolve_package(context, "TTX.Graphics"_view) ==
         package_source);
}

PERIMORTEM_UNIT_TEST(TtxResolution, missing_imports) {
  Resolver resolver;
  Resolver::Context context;
  seed(
      resolver,
      "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import MissingA : Library = (.source = \"missing_a.ttx\");\n"
      "import MissingB : Library = (.source = \"missing_b.ttx\");\n"_view);

  EXPECT_NOT(resolver.resolve_path(context, "unit/root.ttx"_view));

  ASSERT_EQ(context.get_errors().get_size(), Count(2));
  EXPECT_TEXT(error_path(context, 0), "unit/missing_a.ttx"_view);
  EXPECT_TEXT(error_path(context, 1), "unit/missing_b.ttx"_view);
  EXPECT_TEXT(error_message(context, 0), "Imported source file could not be read."_view);
  EXPECT_TEXT(error_message(context, 1), "Imported source file could not be read."_view);
}

PERIMORTEM_UNIT_TEST(TtxResolution, parse_error_retry) {
  Resolver resolver;
  Resolver::Context context;
  seed(resolver, "unit/root.ttx"_view, "dialect : ;\n"_view);

  EXPECT_NOT(resolver.resolve_path(context, "unit/root.ttx"_view));
  EXPECT(context.has_errors());
  context.reset();
  EXPECT_NOT(resolver.resolve_path(context, "unit/root.ttx"_view));
  EXPECT(context.has_errors());

  seed(resolver, "unit/root.ttx"_view, library_source);
  context.reset();
  EXPECT(resolver.resolve_path(context, "unit/root.ttx"_view));
  EXPECT_NOT(context.has_errors());

  const Ttx::Source* source =
      resolver.resolve_source(context, "unit/memory.ttx"_view, library_source);
  ASSERT(source != nullptr);
  EXPECT_TEXT(source->get_source_path(), "unit/memory.ttx"_view);
  EXPECT_TEXT(source->get_source(), library_source);
}

PERIMORTEM_UNIT_TEST(TtxResolution, dialect_mismatch) {
  Resolver resolver;
  Resolver::Context context;
  seed(
      resolver,
      "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Target : Shader = (.source = \"target.ttx\");\n"_view);
  seed(resolver, "unit/target.ttx"_view, library_source);

  EXPECT_NOT(resolver.resolve_path(context, "unit/root.ttx"_view));

  EXPECT(first_error_is(
      context, "unit/root.ttx"_view,
      "Imported source dialect does not match the requested dialect."_view));
}

PERIMORTEM_UNIT_TEST(TtxResolution, cycle_and_rework) {
  Resolver resolver;
  Resolver::Context context;
  seed(
      resolver,
      "unit/a.ttx"_view,
      "dialect : Library;\n"
      "import B : Library = (.source = \"b.ttx\");\n"_view);
  seed(
      resolver,
      "unit/b.ttx"_view,
      "dialect : Library;\n"
      "import A : Library = (.source = \"a.ttx\");\n"_view);

  EXPECT_NOT(resolver.resolve_path(context, "unit/a.ttx"_view));

  EXPECT(first_error_is(
      context, "unit/b.ttx"_view,
      "Import cycle detected while resolving source graph."_view));

  seed(resolver, "unit/b.ttx"_view, library_source);
  context.reset();
  const Ttx::Source* a = resolver.resolve_path(context, "unit/a.ttx"_view);
  ASSERT(a != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Source* b = resolver.resolve_path(context, "unit/b.ttx"_view);
  ASSERT(b != nullptr);
  EXPECT_EQ(a->get_imports().get_size(), Count(1));
  EXPECT_EQ(b->get_imports().get_size(), Count(0));

  seed(resolver, "unit/b.ttx"_view, "dialect : Shader;\n"_view);
  context.reset();
  EXPECT_NOT(resolver.resolve_path(context, "unit/a.ttx"_view));
  EXPECT(first_error_is(
      context, "unit/a.ttx"_view,
      "Imported source dialect does not match the requested dialect."_view));
}

PERIMORTEM_UNIT_TEST(
    TtxResolution,
    package_member_waits) {
  Resolver resolver;
  Resolver::Context context;
  seed(
      resolver,
      "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = TTX::Graphics;\n"
      "import Default2D : Shader = Graphics::Shaders::Default2D;\n"_view);
  seed(
      resolver,
      "tetrodotoxin/packages/graphics/package.ttx"_view,
      package_header);

  EXPECT_NOT(resolver.resolve_path(context, "unit/root.ttx"_view));

  EXPECT(first_error_is(
      context, "unit/root.ttx"_view,
      "Package member import cannot be resolved until package definitions "
      "are available."_view));
}
