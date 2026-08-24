// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/hosting.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness GraphicsHosting = {
  .name = "Tetrodotoxin::Graphics::Hosting"_view,
};

PERIMORTEM_UNIT_TEST(GraphicsHosting, negotiates_real_library_types) {
  static constexpr View::Bytes source =
      "// Graphics hosting contract.\n"
      "dialect : Library;\n"
      "public Transform : struct { public state x : R64; }\n"
      "public Host : struct {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public Sprite : object {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "  public state texture : U64;\n"
      "}\n"
      "public WrongType : object {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : U64;\n"
      "}\n"
      "public HiddenState : object {\n"
      "  public state transform : Transform;\n"
      "  private state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public InlineValue : struct {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}"_view;

  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto monograph = workspace.interpret_source(
      errors, "Graphics"_view, "graphics-hosting.ttx"_view, source);
  ASSERT(monograph);
  ASSERT(errors.is_empty());

  const Abstract& requirement = monograph->resolve_context("Host"_view);
  const Abstract& sprite = monograph->resolve_context("Sprite"_view);
  const Abstract& wrong_type = monograph->resolve_context("WrongType"_view);
  const Abstract& hidden_state = monograph->resolve_context("HiddenState"_view);
  const Abstract& inline_value = monograph->resolve_context("InlineValue"_view);
  Graphics::Hosting hosting;

  EXPECT(
      hosting.negotiate(requirement, requirement) ==
      Ttx::Concept::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, sprite) ==
      Ttx::Concept::Interface::Relation::Satisfied);
  EXPECT(
      hosting.negotiate(requirement, wrong_type) ==
      Ttx::Concept::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, hidden_state) ==
      Ttx::Concept::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, inline_value) ==
      Ttx::Concept::Interface::Relation::Rejected);
}
