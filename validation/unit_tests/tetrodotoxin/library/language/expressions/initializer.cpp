// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
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

PERIMORTEM_UNIT_TEST(InitializerTests, omitted_and_supplied) {
  static constexpr View::Bytes source =
      "// Initializer test.\n"
      "dialect : Library;\n"
      "public Defaults : object { public cache : Bool; public state enabled : "
      "Bool = false; }\n"
      "public Required : object {\n"
      "  public state first : Unsigned_64;\n"
      "  private state hidden : Bool = false;\n"
      "  expose state second : Bool = false;\n"
      "}\n"
      "public empty : Defaults = new[Defaults];\n"
      "public configured : Required = "
      "new[Required](.second = true, .first = 4,);\n"
      "public positional : Required = new[Required](5, true);"_view;
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
  const auto& configured_field = static_cast<const Language::Field&>(
      source_type.resolve_context("configured"_view));
  const auto& positional_field = static_cast<const Language::Field&>(
      source_type.resolve_context("positional"_view));
  auto empty_initializer = empty_field.get_initializer();
  auto configured_initializer = configured_field.get_initializer();
  auto positional_initializer = positional_field.get_initializer();
  ASSERT(empty_initializer);
  ASSERT(configured_initializer);
  ASSERT(positional_initializer);
  ASSERT(empty_initializer->is<Language::Expressions::Initializer>());
  ASSERT(configured_initializer->is<Language::Expressions::Initializer>());
  ASSERT(positional_initializer->is<Language::Expressions::Initializer>());
  const auto& empty = static_cast<const Language::Expressions::Initializer&>(
      *empty_initializer);
  const auto& configured =
      static_cast<const Language::Expressions::Initializer&>(
          *configured_initializer);
  const auto& positional =
      static_cast<const Language::Expressions::Initializer&>(
          *positional_initializer);
  EXPECT(&empty.get_type() == &defaults);
  EXPECT(&configured.get_type() == &required);
  EXPECT(&positional.get_type() == &required);

  ASSERT(empty.get_completed_values());
  ASSERT(configured.get_completed_values());
  ASSERT(positional.get_completed_values());
  EXPECT_EQ(empty.get_completed_values()->get_layout().get_size(), Count(1));
  const Layout& configured_values =
      configured.get_completed_values()->get_layout();
  const Layout& positional_values =
      positional.get_completed_values()->get_layout();
  ASSERT_EQ(configured_values.get_size(), Count(3));
  ASSERT_EQ(positional_values.get_size(), Count(3));
  auto configured_first = configured_values.get_abstract(0);
  auto configured_hidden = configured_values.get_abstract(1);
  auto configured_second = configured_values.get_abstract(2);
  ASSERT(
      configured_first &&
      configured_first->is<Language::Constants::Unsigned>());
  ASSERT(
      configured_hidden && configured_hidden->is<Language::Constants::False>());
  ASSERT(
      configured_second && configured_second->is<Language::Constants::True>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*configured_first)
          .get_value(),
      Unsigned_64(4));
  auto positional_first = positional_values.get_abstract(0);
  auto positional_hidden = positional_values.get_abstract(1);
  auto positional_second = positional_values.get_abstract(2);
  ASSERT(
      positional_first &&
      positional_first->is<Language::Constants::Unsigned>());
  ASSERT(
      positional_hidden && positional_hidden->is<Language::Constants::False>());
  ASSERT(
      positional_second && positional_second->is<Language::Constants::True>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*positional_first)
          .get_value(),
      Unsigned_64(5));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, descendant_private_field) {
  static constexpr View::Bytes source =
      "// Descendant Object initialization.\n"
      "dialect : Library;\n"
      "public Owner : object {\n"
      "  private state hidden : Bool = false;\n"
      "  public Builder : struct {\n"
      "    private value : Owner = new[Owner](.hidden = true);\n"
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

PERIMORTEM_UNIT_TEST(InitializerTests, inferred_carries_object_type) {
  static constexpr View::Bytes source =
      "// Inferred initializer test.\n"
      "dialect : Library;\n"
      "public Session : object { public state active : Bool; }\n"
      "public inferred := new[Session];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& source_type = monograph->get_source();
  const Abstract& session = source_type.resolve_context("Session"_view);
  const auto& inferred = static_cast<const Language::Field&>(
      source_type.resolve_context("inferred"_view));
  EXPECT(&inferred.get_type() == &session);
  auto initializer = inferred.get_initializer();
  ASSERT(initializer && initializer->is<Language::Expressions::Initializer>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, wrong_type_rejected) {
  static constexpr View::Bytes source =
      "// Wrong initializer Type test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Bool; }\n"
      "public invalid : Bool = new[Session];"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, unknown_name_rejected) {
  static constexpr View::Bytes source =
      "// Unknown initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public invalid : Session = new[Session](.missing = 1);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, private_name_rejected) {
  static constexpr View::Bytes source =
      "// Private initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { private state hidden : Bool = false; }\n"
      "public invalid : Session = new[Session](.hidden = true);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, duplicate_name_rejected) {
  static constexpr View::Bytes source =
      "// Duplicate initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public invalid : Session = "
      "new[Session](.value = 1, .value = 2);"_view;
  EXPECT(rejects_interpretation(
      source, "Duplicate name in one Library Pack."_view));
}

PERIMORTEM_UNIT_TEST(InitializerTests, mixed_pack_rejected) {
  static constexpr View::Bytes source =
      "// Mixed initializer Layout test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public invalid : Session = new[Session](1, .value = 2);"_view;
  EXPECT(rejects_interpretation(
      source,
      "Positional and named entries cannot share one Library Pack."_view));
}

PERIMORTEM_UNIT_TEST(InitializerTests, omission_uses_type_default) {
  static constexpr View::Bytes source =
      "// Omitted initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public created : Session = new[Session];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& created = static_cast<const Language::Field&>(
      monograph->get_source().resolve_context("created"_view));
  auto value = created.get_initializer();
  ASSERT(value && value->is<Language::Expressions::Initializer>());
  const auto& initializer =
      static_cast<const Language::Expressions::Initializer&>(*value);
  ASSERT(initializer.get_completed_values());
  auto field_value =
      initializer.get_completed_values()->get_layout().get_abstract(0);
  ASSERT(field_value && field_value->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*field_value)
          .get_value(),
      Unsigned_64(0));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, empty_argument_pack_rejected) {
  static constexpr View::Bytes source =
      "// Empty initializer argument test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public invalid : Session = new[Session]();"_view;
  EXPECT(rejects_interpretation(
      source, "Object initializer arguments cannot be empty."_view));
}

PERIMORTEM_UNIT_TEST(InitializerTests, nested_defaults) {
  static constexpr View::Bytes source =
      "// Nested Object default test.\n"
      "dialect : Library;\n"
      "public Inner : object { private state value : Unsigned_64; }\n"
      "public Outer : object {\n"
      "  private state inner : Inner; private state enabled : Bool;\n"
      "}\n"
      "public created : Outer = new[Outer];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& inner = monograph->get_source().resolve_context("Inner"_view);
  const auto& created = static_cast<const Language::Field&>(
      monograph->get_source().resolve_context("created"_view));
  auto value = created.get_initializer();
  ASSERT(value && value->is<Language::Expressions::Initializer>());
  const auto& initializer =
      static_cast<const Language::Expressions::Initializer&>(*value);
  ASSERT(initializer.get_completed_values());
  const Layout& values = initializer.get_completed_values()->get_layout();
  ASSERT_EQ(values.get_size(), Count(2));
  auto nested = values.get_abstract(0);
  auto enabled = values.get_abstract(1);
  ASSERT(nested && nested->is<Language::Expressions::Initializer>());
  ASSERT(enabled && enabled->is<Language::Constants::False>());
  EXPECT(
      &static_cast<const Language::Expressions::Initializer&>(*nested)
           .get_type() == &inner);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, input_type_rejected) {
  static constexpr View::Bytes source =
      "// Initializer input Type test.\n"
      "dialect : Library;\n"
      "public Session : object { public state value : Unsigned_64; }\n"
      "public invalid : Session = new[Session](.value = false);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, const_field_override_rejected) {
  static constexpr View::Bytes source =
      "// Const initializer ownership test.\n"
      "dialect : Library;\n"
      "public Session : object {\n"
      "  public const value : Unsigned_64 = 1;\n"
      "}\n"
      "public invalid : Session = new[Session](.value = 2);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, mandatory_cycle_rejected) {
  static constexpr View::Bytes static_override =
      "// Static initializer ownership test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64 = 1; }\n"
      "public invalid : Session = new[Session](.value = 2);"_view;
  EXPECT(rejects_link_without_publication(static_override));

  static constexpr View::Bytes source =
      "// Initializer cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Node = new[Node]; }\n"
      "public invalid : Node = new[Node];"_view;
  EXPECT(rejects_link_without_publication(source));

  static constexpr View::Bytes missing_default =
      "// Missing default cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Node; }\n"
      "public invalid : Node = new[Node];"_view;
  EXPECT(rejects_link_without_publication(missing_default));

  static constexpr View::Bytes optional =
      "// Optional default cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Option[Node]; }\n"
      "public valid : Node = new[Node];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, optional);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  const auto& valid = static_cast<const Language::Field&>(
      monograph->get_source().resolve_context("valid"_view));
  auto initializer = valid.get_initializer();
  ASSERT(initializer && initializer->is<Language::Expressions::Initializer>());
  const auto& value =
      static_cast<const Language::Expressions::Initializer&>(*initializer);
  ASSERT(value.get_completed_values());
  auto next = value.get_completed_values()->get_layout().get_abstract(0);
  ASSERT(next && next->is<Language::Constants::Option>());
  EXPECT(errors.is_empty());
}
