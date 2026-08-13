// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "InitializerTest"_view, "initializer.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(
    View::Bytes source,
    View::Bytes diagnostic = {}) -> Bool {
  Workspace workspace;
  Errors errors;
  BAIL_IF(interpret(workspace, errors, source) || errors.is_empty());
  if (diagnostic.is_empty()) {
    return True;
  }

  Perimortem::Memory::Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), diagnostic) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

static auto rejects_link_without_publication(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || workspace.link(errors) || errors.is_empty()) {
    return False;
  }

  return &workspace.resolve_context("InitializerTest"_view) ==
         &Invalid::get_invalid();
}

static Harness InitializerTests = {
  .name = "Tetrodotoxin::Library::Language::Expressions::Initializer"_view,
};

PERIMORTEM_UNIT_TEST(InitializerTests, empty_and_supplied) {
  static constexpr View::Bytes source =
      "// Initializer test.\n"
      "dialect : Library;\n"
      "public Defaults : object { public enabled : Bool = false; }\n"
      "public Required : object {\n"
      "  public first : Unsigned_64;\n"
      "  private hidden : Bool = false;\n"
      "  expose state second : Bool = false;\n"
      "}\n"
      "public empty : Defaults = new;\n"
      "public parenthesized : Defaults = new();\n"
      "public configured : Required = new(.second = true, .first = 4,);"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& source_type = monograph->get_source();
  const auto& defaults = static_cast<const Language::Types::Object&>(
      source_type.resolve_context("Defaults"_view));
  const auto& required = static_cast<const Language::Types::Object&>(
      source_type.resolve_context("Required"_view));
  const auto& empty_field = static_cast<const Language::Field&>(
      source_type.resolve_context("empty"_view));
  const auto& parenthesized_field = static_cast<const Language::Field&>(
      source_type.resolve_context("parenthesized"_view));
  const auto& configured_field = static_cast<const Language::Field&>(
      source_type.resolve_context("configured"_view));
  auto empty_initializer = empty_field.get_initializer();
  auto parenthesized_initializer = parenthesized_field.get_initializer();
  auto configured_initializer = configured_field.get_initializer();
  ASSERT(empty_initializer);
  ASSERT(parenthesized_initializer);
  ASSERT(configured_initializer);
  ASSERT(empty_initializer->is<Language::Expressions::Initializer>());
  ASSERT(parenthesized_initializer->is<Language::Expressions::Initializer>());
  ASSERT(configured_initializer->is<Language::Expressions::Initializer>());
  const auto& empty = static_cast<const Language::Expressions::Initializer&>(
      *empty_initializer);
  const auto& parenthesized =
      static_cast<const Language::Expressions::Initializer&>(
          *parenthesized_initializer);
  const auto& configured =
      static_cast<const Language::Expressions::Initializer&>(
          *configured_initializer);
  EXPECT(&empty.get_type() == &defaults);
  EXPECT(&parenthesized.get_type() == &defaults);
  EXPECT(&configured.get_type() == &required);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, descendant_private_field) {
  static constexpr View::Bytes source =
      "// Descendant Object initialization.\n"
      "dialect : Library;\n"
      "public Owner : object {\n"
      "  private hidden : Bool = false;\n"
      "  public Builder : struct {\n"
      "    private value : Owner = new(.hidden = true);\n"
      "  }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, inferred_rejected) {
  static constexpr View::Bytes source =
      "// Inferred initializer test.\n"
      "dialect : Library;\n"
      "public Session : object {}\n"
      "public invalid := new;"_view;
  EXPECT(rejects_interpretation(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, wrong_type_rejected) {
  static constexpr View::Bytes source =
      "// Wrong initializer Type test.\n"
      "dialect : Library;\n"
      "public invalid : Bool = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, unknown_name_rejected) {
  static constexpr View::Bytes source =
      "// Unknown initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.missing = 1);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, private_name_rejected) {
  static constexpr View::Bytes source =
      "// Private initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { private hidden : Bool = false; }\n"
      "public invalid : Session = new(.hidden = true);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, duplicate_name_rejected) {
  static constexpr View::Bytes source =
      "// Duplicate initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.value = 1, .value = 2);"_view;
  EXPECT(rejects_interpretation(
      source, "Duplicate name in one Library Pack."_view));
}

PERIMORTEM_UNIT_TEST(InitializerTests, mixed_pack_rejected) {
  static constexpr View::Bytes source =
      "// Mixed initializer Layout test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(1, .value = 2);"_view;
  EXPECT(rejects_interpretation(
      source,
      "Positional and named entries cannot share one Library Pack."_view));
}

PERIMORTEM_UNIT_TEST(InitializerTests, missing_field_rejected) {
  static constexpr View::Bytes source =
      "// Missing initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, input_type_rejected) {
  static constexpr View::Bytes source =
      "// Initializer input Type test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.value = false);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, const_field_override_rejected) {
  static constexpr View::Bytes source =
      "// Const initializer ownership test.\n"
      "dialect : Library;\n"
      "public Session : object {\n"
      "  public const value : Unsigned_64 = 1;\n"
      "}\n"
      "public invalid : Session = new(.value = 2);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, mandatory_cycle_rejected) {
  static constexpr View::Bytes source =
      "// Initializer cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private next : Node = new; }\n"
      "public invalid : Node = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}
