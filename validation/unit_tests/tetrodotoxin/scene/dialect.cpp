// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/source/library.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness SceneDialect = {
  .name = "Tetrodotoxin::Scene::Dialect"_view,
};

// Produces a parsed Library child whose retained import fails during the
// enclosing Workspace link step. This keeps the failure inside the one atomic
// source transaction instead of mutating a published Monograph afterward.
class RejectingLibraryDialect : public Library::Dialect {
 public:
  using Library::Dialect::Dialect;

  auto interpret(
      Cursor& source_cursor,
      const Documentation& documentation,
      const Anchor& source_anchor,
      Abstract& context)
      -> Option<Tetrodotoxin::Language::Monograph&> override {
    auto interpreted = Library::Dialect::interpret(
        source_cursor, documentation, source_anchor, context);
    if (!interpreted) {
      return {};
    }

    Errors parser_errors;
    Tokenizer tokenizer(
        source_cursor.get_arena(), "using Missing;"_view,
        "scene-child.ttx"_view);
    Ttx::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, parser_errors, associations);
    auto& monograph = static_cast<Library::Language::Monograph&>(*interpreted);
    Library::Interpreter::Source::Library::parse(
        monograph.get_source(), cursor);
    if (!cursor.matches(Code::Type::Terminal) || !parser_errors.is_empty()) {
      source_cursor.create_error(
          "The test Library child could not be prepared."_view);
      return {};
    }

    return interpreted;
  }
};

PERIMORTEM_UNIT_TEST(SceneDialect, exact_layers) {
  static constexpr View::Bytes source = "//\ndialect : Scene;"_view;
  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  ASSERT(library);
  auto dialect = toolchain.install<Scene::Dialect>("Scene"_view, *library);
  ASSERT(dialect);
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Empty"_view, "empty-scene.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Scene::Language::Monograph>());
  const auto& immutable =
      static_cast<const Scene::Language::Monograph&>(*interpreted);
  const auto& child = immutable.get_library();
  Allocator::Arena other_domain;
  Library::Dialect other_library;
  Scene::Dialect other_scene(*library);
  ASSERT(immutable.get_layer(*dialect));
  ASSERT(immutable.get_layer(*library));
  EXPECT(&*immutable.get_layer(*dialect) == &immutable);
  EXPECT(&*immutable.get_layer(*library) == &child);
  EXPECT_NOT(immutable.get_layer(other_scene));
  EXPECT_NOT(immutable.get_layer(other_library));
  EXPECT(&child.resolve_context("Empty"_view) == &immutable);
  ASSERT_EQ(
      child.get_documentation().line_count(),
      immutable.get_documentation().line_count());
  EXPECT_TEXT(
      child.get_documentation().get_line(0),
      immutable.get_documentation().get_line(0));
  EXPECT(&workspace.resolve_context("Empty"_view) == &immutable);
  EXPECT(&workspace.resolve_context("Library"_view) == &Invalid::get_invalid());
  EXPECT(child.get_source().is_linked());
  EXPECT(child.get_source().is_finalized());
  EXPECT(&workspace.resolve_context("Empty"_view) == &immutable);
  EXPECT(&workspace.resolve_context("Library"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SceneDialect, child_rejection) {
  static constexpr View::Bytes scene_source = "//\ndialect : Scene;"_view;
  Environment::Toolchain toolchain;
  auto library = toolchain.install<RejectingLibraryDialect>("Library"_view);
  ASSERT(library);
  ASSERT(toolchain.install<Scene::Dialect>("Scene"_view, *library));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Broken"_view, "broken-scene.ttx"_view, scene_source);

  EXPECT_NOT(interpreted);
  EXPECT(workspace.resolve_context("Broken"_view)
             .is<Scene::Language::Monograph>());
  EXPECT_EQ(errors.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(SceneDialect, delayed_declarations) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Scene;\n"
      "signal later;"_view;
  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  ASSERT(library);
  ASSERT(toolchain.install<Scene::Dialect>("Scene"_view, *library));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Rejected"_view, "scene-declaration.ttx"_view, source);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  auto retained = workspace.get_monograph("scene-declaration.ttx"_view);
  ASSERT(retained && retained->is<Scene::Language::Monograph>());
  EXPECT(&workspace.resolve_context("Rejected"_view) == &*retained);
}

PERIMORTEM_UNIT_TEST(SceneDialect, library_declarations) {
  static constexpr View::Bytes source =
      "// Scene with ordinary CPU meaning.\n"
      "dialect : Scene;\n"
      "public Item : struct {\n"
      "  public state value : U64 = 7;\n"
      "}\n"
      "public read : func = [] -> U64 : return 7;"_view;
  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  ASSERT(library);
  ASSERT(toolchain.install<Scene::Dialect>("Scene"_view, *library));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Owned"_view, "owned-scene.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Scene::Language::Monograph>());
  const auto& scene =
      static_cast<const Scene::Language::Monograph&>(*interpreted);
  EXPECT(scene.resolve_context("Item"_view)
             .resolve()
             .is<Library::Language::Types::Structure>());
  EXPECT(scene.resolve_call(scene, "read"_view)
             .resolve()
             .is<Library::Language::Function>());
  EXPECT(scene.get_library().get_source().is_linked());
  EXPECT(scene.get_library().get_source().is_finalized());
  EXPECT(errors.is_empty());
}
