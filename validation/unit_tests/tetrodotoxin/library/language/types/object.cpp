// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/object.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
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
      errors, "ObjectTest"_view, "object.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Bool linked = workspace.link(errors);
  return !linked && !errors.is_empty();
}

static auto rejects_finalize(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || !workspace.link(errors)) {
    return False;
  }

  Bool finalized = workspace.finalize(errors);
  return !finalized && !errors.is_empty();
}

static Harness ObjectTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Object"_view,
};

PERIMORTEM_UNIT_TEST(ObjectTests, field_writability) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object {\n"
      "  public open : Bool;\n"
      "  private closed : Bool;\n"
      "  expose state observed : Bool = false;\n"
      "  private state hidden_state : Bool = false;\n"
      "  public const fixed : Bool = false;\n"
      "  private const hidden_const : Bool = false;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& selected = monograph->resolve_context("Session"_view);
  ASSERT(selected.is<Language::Types::Object>());
  const auto& object = static_cast<const Language::Types::Object&>(selected);
  auto fields = object.get_addressables();

  // Every authored mode remains a policy on the real Field identity rather
  // than creating another mutable binding.
  auto field = fields.begin();
  ASSERT(field != fields.end());
  const auto& open = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& closed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& observed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& hidden_state =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& fixed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& hidden_const =
      static_cast<const Language::Field&>((*field).get());
  EXPECT(open.get_writability() == Language::Field::Writability::Full);
  EXPECT(closed.get_writability() == Language::Field::Writability::Full);
  EXPECT(observed.get_writability() == Language::Field::Writability::Internal);
  EXPECT(
      hidden_state.get_writability() == Language::Field::Writability::Internal);
  EXPECT(fixed.get_writability() == Language::Field::Writability::Init);
  EXPECT(hidden_const.get_writability() == Language::Field::Writability::Init);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, exact_collision_domain) {
  static constexpr Static::Vector<View::Bytes, 3> accepted = {{
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { private state value : Bool = false; public value : func = [] -> [] {} }"_view,
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private value : func = [] -> [] {} }"_view,
  }};
  for (Count i = 0; i < accepted.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, accepted[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));
    EXPECT(errors.is_empty());
  }

  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false; private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : object {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : struct {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : enum[Unsigned_8] {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : func = [] -> [] {}"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_interpretation(rejected[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 10> sources = {{
    "// Object test.\ndialect : Library; public Session object {}"_view,
    "// Object test.\ndialect : Library; public Session : managed {}"_view,
    "// Object test.\ndialect : Library; public Session : object { state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { public state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { public const value : Bool; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state Value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_exposure_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Object test.\ndialect : Library; private Hidden : object {} public reveal : func = [Hidden] -> [] {}"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : struct { public hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : object { public hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : object { public reveal : func = [] -> Hidden {} }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_surface_retained_locally) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "private Hidden : object {}\n"
      "public Holder : object {\n"
      "  private hidden : Hidden;\n"
      "  private reveal : func = [Hidden] -> Hidden {}\n"
      "}\n"
      "private root : func = [Hidden] -> Hidden {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  const auto& source_type = monograph->get_source();
  auto types = source_type.get_types();
  auto source_callables = source_type.get_callables();
  ASSERT(types != types.end());
  ASSERT((*types).get().is<Language::Types::Object>());
  const Abstract& hidden = (*types).get();
  ASSERT(source_callables != source_callables.end());
  ASSERT((*source_callables).get().is<Language::Function>());
  const auto& root =
      static_cast<const Language::Function&>((*source_callables).get());
  const Abstract& holder = monograph->resolve_context("Holder"_view);
  EXPECT(hidden.is<Language::Types::Object>());
  ASSERT(holder.is<Language::Types::Object>());
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(&holder.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&holder.resolve_context("reveal"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("root"_view) == &Invalid::get_invalid());
  const auto& holder_object =
      static_cast<const Language::Types::Object&>(holder);
  auto fields = holder_object.get_addressables();
  auto callables = holder_object.get_callables();
  ASSERT(fields != fields.end());
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>((*callables).get());
  EXPECT(&reveal.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&reveal.resolve_context("Hidden"_view) == &hidden);
  EXPECT(&root.resolve_context("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, inherited_initializer_mismatch) {
  static constexpr View::Bytes source =
      "// Object initializer test.\n"
      "dialect : Library;\n"
      "public Session : object { private state value : Unsigned_8 = false; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Abstract& selected = monograph->resolve_context("Session"_view);
  ASSERT(selected.is<Language::Types::Object>());
  const auto& object = static_cast<const Language::Types::Object&>(selected);
  auto authored_fields = object.get_addressables();
  ASSERT(authored_fields != authored_fields.end());
  const auto& authored_field =
      static_cast<const Language::Field&>((*authored_fields).get());
  auto authored_initializer = authored_field.get_initializer();
  ASSERT(authored_initializer);

  EXPECT_NOT(workspace.link(errors));
  ASSERT(authored_field.get_initializer());
  EXPECT(&*authored_field.get_initializer() == &*authored_initializer);
  auto diagnostics = monograph->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(source),
      "false"_view);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, link_failure_keeps_publication_empty) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object { expose state value : Bool = missing; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  EXPECT_NOT(workspace.link(errors));
  EXPECT(
      &workspace.resolve_context("ObjectTest"_view) == &Invalid::get_invalid());
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, missing_type_rejected) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object { private state value : Missing = false; }"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(ObjectTests, inferred_object_identity) {
  static constexpr View::Bytes source =
      "// Object inference test.\n"
      "dialect : Library;\n"
      "public Child : object {}\n"
      "public Holder : object {\n"
      "  private child : Child;\n"
      "  private copy := child;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Abstract& child_identity = monograph->resolve_context("Child"_view);
  const Abstract& holder_identity = monograph->resolve_context("Holder"_view);
  ASSERT(child_identity.is<Language::Types::Object>());
  ASSERT(holder_identity.is<Language::Types::Object>());
  const auto& child =
      static_cast<const Language::Types::Object&>(child_identity);
  const auto& holder =
      static_cast<const Language::Types::Object&>(holder_identity);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  auto fields = holder.get_addressables();
  ASSERT(fields != fields.end());
  const auto& child_field =
      static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& copy_field = static_cast<const Language::Field&>((*fields).get());
  EXPECT(&child_field.get_type() == &child);
  EXPECT(&copy_field.get_type() == &child);
  EXPECT_NOT(copy_field.get_type_access());
  ASSERT(copy_field.get_initializer());
  ASSERT(copy_field.get_initializer()->is<Language::Identifier>());
  const auto& identifier =
      static_cast<const Language::Identifier&>(*copy_field.get_initializer());
  ASSERT(identifier.get_addressable());
  EXPECT(&*identifier.get_addressable() == &child_field);
  EXPECT(errors.is_empty());
}
