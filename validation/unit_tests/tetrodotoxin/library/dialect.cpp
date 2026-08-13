// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/assignment.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/foreign/function.hpp"
#include "tetrodotoxin/library/language/foreign/state.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto find_return(const Language::Function& function)
    -> Option<const Language::Flow::Return&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Reference<Abstract>& statement : body->get_statements()) {
    auto returned = statement.get().select<Language::Flow::Return>();
    if (returned) {
      return *returned;
    }
  }

  return {};
}

class EmptyRegistry : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "EmptyRegistry"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class EmbeddedResource final : public Tetrodotoxin::Language::Resource {
 public:
  constexpr auto get_value() const -> View::Bytes override {
    return "0123456789ABCDEF"_view;
  }
};

class ResourceRegistry final : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "ResourceRegistry"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return route == "$[resource/hello.txt]"_view
               ? static_cast<const Abstract&>(resource)
               : static_cast<const Abstract&>(Invalid::get_invalid());
  }

 private:
  EmbeddedResource resource;
};

class FutureType : public Type {
 public:
  constexpr FutureType(View::Bytes name = "Future"_view) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

class QualifiedContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Types"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == qualified.get_name()) {
      return qualified;
    }

    return Invalid::get_invalid();
  }

 private:
  FutureType qualified{"Qualified"_view};
};

class NonTypeFact : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Fact"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class AliasContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "AliasContext"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == types.get_name()) {
      return types;
    }
    if (route == outer.get_name()) {
      return outer;
    }
    if (route == fact.get_name()) {
      return fact;
    }

    return Invalid::get_invalid();
  }

 private:
  QualifiedContext types;
  FutureType outer{"Outer"_view};
  NonTypeFact fact;
};

static auto import_library(
    Workspace& workspace,
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes source) -> Option<Language::Monograph&> {
  auto imported =
      workspace.interpret_source(errors, semantic_name, semantic_name, source);
  if (!imported || !imported->is<Language::Monograph>()) {
    return {};
  }

  Bool linked = workspace.link(errors);
  if (!linked) {
    return {};
  }

  Bool finalized = workspace.finalize(errors);
  if (!finalized) {
    return {};
  }

  return static_cast<Language::Monograph&>(*imported);
}

static auto rejects_library_source(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return False;
  }

  auto interpreted = workspace.interpret_source(
      errors, "RejectedLibrary"_view, "rejected-library.ttx"_view, source);
  if (!interpreted) {
    return !errors.is_empty();
  }
  if (!workspace.link(errors)) {
    return !errors.is_empty();
  }
  if (!workspace.finalize(errors)) {
    return !errors.is_empty();
  }

  return False;
}

static Harness DialectTests = {
  .name = "Tetrodotoxin::Library::Dialect"_view,
};

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  auto fields = composite.get_addressables();
  for (auto field = fields.begin(); field != fields.end(); ++field) {
    const Abstract& candidate = (*field).get();
    if (candidate.get_name() == name && candidate.is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate);
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  auto functions = composite.get_callables();
  for (auto function = functions.begin(); function != functions.end();
       ++function) {
    const Abstract& candidate = (*function).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(DialectTests, nested_missing_type_reports_authored_route) {
  static constexpr View::Bytes source =
      "public Packet : struct { public missing : Missing; }\n"
      "public later : func = [] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "phase-cascade.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  ASSERT_NOT(monograph.link());
  ASSERT_EQ(monograph.get_diagnostics().get_size(), Count(1));
  ASSERT(monograph.get_diagnostics().get_data()[0].get_anchor());
  EXPECT_TEXT(
      monograph.get_diagnostics()
          .get_data()[0]
          .get_anchor()
          ->get_span()
          .caculate_text(source),
      "Missing"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, source_field_failures_are_reported) {
  static constexpr Static::Vector<View::Bytes, 6> sources = {{
    "public broken : Missing;\npublic later : func = [] -> Void {}"_view,
    "public broken : Bool = absent;\npublic later : func = [] -> Void {}"_view,
    "public const broken : Bool = 1;\n"
    "public later : func = [] -> Void {}"_view,
    "public const broken : Unsigned_8 = 256;\n"
    "public later : func = [] -> Void {}"_view,
    "public dynamic : Bool = false;\n"
    "public const broken := dynamic;"_view,
    "public Packet : struct {\n"
    "  public dynamic : Bool = false;\n"
    "  public const broken := dynamic;\n"
    "}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Allocator::Arena arena;
    EmptyRegistry registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, sources[i], "source-field-failure.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto interpreted = dialect.interpret(
        arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
        registry);
    ASSERT(interpreted && interpreted->is<Language::Monograph>());
    auto& monograph = static_cast<Language::Monograph&>(*interpreted);
    ASSERT_NOT(monograph.link());
    EXPECT_NOT(monograph.get_diagnostics().is_empty());
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, top_level_self_is_rejected_at_registration) {
  EXPECT(rejects_library_source(
      "// Top level Self registration.\n"
      "dialect : Library;\n"
      "public invalid : func = [self] -> Void {}"_view));
}

PERIMORTEM_UNIT_TEST(DialectTests, source_rejects_instance_state) {
  EXPECT(rejects_library_source(
      "// Source state rejection.\n"
      "dialect : Library;\n"
      "public state invalid : Bool;"_view));
}

PERIMORTEM_UNIT_TEST(DialectTests, source_alias_identity_and_visibility) {
  static constexpr View::Bytes source =
      "// Hidden documentation.\n"
      "private Hidden : struct {}\n"
      "// Local documentation.\n"
      "public PublicAlias : alias = Hidden;\n"
      "private PrivateAlias : alias = PublicAlias;"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source-alias.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  const auto& source_type = monograph.get_source();
  auto types = source_type.get_types();
  auto type = types.begin();
  ASSERT(type != types.end());
  const Abstract& hidden = (*type).get();
  ++type;
  ASSERT(type != types.end());
  const Abstract& public_identity = (*type).get();
  ++type;
  ASSERT(type != types.end());
  const Abstract& private_identity = (*type).get();
  ++type;
  EXPECT(type == types.end());
  ASSERT(hidden.is<Language::Types::Structure>());
  ASSERT(public_identity.is<Alias>());
  ASSERT(private_identity.is<Alias>());
  const auto& public_alias = static_cast<const Alias&>(public_identity);
  const auto& private_alias = static_cast<const Alias&>(private_identity);

  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  EXPECT(&public_alias.resolve() == &hidden);
  EXPECT(&private_alias.resolve() == &hidden);
  EXPECT_EQ(public_alias.get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      public_alias.get_documentation().get_line(0),
      "Local documentation."_view);
  EXPECT_TEXT(
      public_alias.get_documentation().get_line(1),
      "Hidden documentation."_view);
  EXPECT(&monograph.resolve_context("PublicAlias"_view) == &public_identity);
  EXPECT(
      &monograph.resolve_context("PrivateAlias"_view) ==
      &Invalid::get_invalid());
  EXPECT(&monograph.resolve_context("PublicAlias"_view) == &public_identity);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(DialectTests, rejected_source_alias_is_atomic) {
  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Rejected documentation.\npublic Broken : alias Bool;"_view,
    "public Bool : alias = Unsigned_8;"_view,
    "public Outer : alias = Unsigned_8;"_view,
    "public Broken : alias = Bool"_view,
    "public First : alias = Bool; public First : alias = Unsigned_8;"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Allocator::Arena arena;
    AliasContext registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, rejected[i], "rejected-source-alias.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto interpreted = dialect.interpret(
        arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
        registry);
    EXPECT_NOT(interpreted);
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, source_alias_binding_is_delayed) {
  static constexpr Static::Vector<View::Bytes, 3> rejected = {{
    "public Broken : alias = Missing;"_view,
    "public Broken : alias = Fact;"_view,
    "public Self : alias = Self;"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Allocator::Arena arena;
    AliasContext registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, rejected[i], "delayed-source-alias.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto interpreted = dialect.interpret(
        arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
        registry);
    ASSERT(interpreted && interpreted->is<Language::Monograph>());
    auto& monograph = static_cast<Language::Monograph&>(*interpreted);
    EXPECT(errors.is_empty());
    EXPECT_NOT(monograph.link());
    EXPECT_NOT(monograph.get_diagnostics().is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, focused_fixture_rejections) {
  struct Rejection {
    View::Bytes path;
    View::Bytes message;
    View::Bytes source_line;
  };
  static constexpr Static::Vector<Rejection, 12> rejections = {{
    Rejection{
      "validation/data/ttx/library/dialect_led_callable.ttx"_view,
      "Definitions require one authored visibility before their name."_view,
      "Library legacy[] -> Void {"_view,
    },
    {
      "validation/data/ttx/library/duplicate_name.ttx"_view,
      "Library member collides with an occupied Composite category."_view,
      "public duplicate : Unsigned_64 = 2;"_view,
    },
    {
      "validation/data/ttx/library/foreign_named_scope.ttx"_view,
      "Library members require a Type, `alias`, `enum`, `struct`, "
      "`object`, `func`, or inferred initializer qualifier."_view,
      "private C : foreign {"_view,
    },
    {
      "validation/data/ttx/library/new_without_expected_type.ttx"_view,
      "An inferred Library Field cannot use `new`."_view,
      "private inferred := new;"_view,
    },
    {
      "validation/data/ttx/library/ordinary_bodyless.ttx"_view,
      "Library Function signatures require a body beginning with `{`."_view,
      "public missing_body : func = [] -> Unsigned_64;"_view,
    },
    {
      "validation/data/ttx/library/foreign_undeclared_symbol.ttx"_view,
      "Library invocation did not find its registered Callable."_view,
      "foreign -> missing();"_view,
    },
    {
      "validation/data/ttx/library/foreign_with_body.ttx"_view,
      "Foreign Function declarations cannot contain an authored body."_view,
      "public func illegal_definition[] -> Void {"_view,
    },
    {
      "validation/data/ttx/library/foreign_const.ttx"_view,
      "Foreign const declarations require a loader or embedding contract."_view,
      "public const imported_constant : Unsigned_64;"_view,
    },
    {
      "validation/data/ttx/library/foreign_private_state.ttx"_view,
      "Private Foreign State is unreachable from its parent Library."_view,
      "private state hidden : Unsigned_64;"_view,
    },
    {
      "validation/data/ttx/library/foreign_private_function.ttx"_view,
      "Private Foreign Functions are unreachable from their parent Library."_view,
      "private func hidden[] -> Void;"_view,
    },
    {
      "validation/data/ttx/library/foreign_exposed_function.ttx"_view,
      "Foreign Functions do not accept `expose` visibility."_view,
      "expose func visible[] -> Void;"_view,
    },
    {
      "validation/data/ttx/library/foreign_exposed_write.ttx"_view,
      "Assignment target is not one writable Library Addressable."_view,
      "foreign.observed = 1;"_view,
    },
  }};

  for (Count i = 0; i < rejections.get_size(); i++) {
    const Rejection& rejection = rejections[i];
    auto source = File::read(rejection.path);
    ASSERT(source);

    Workspace workspace;
    Errors errors;
    ASSERT(workspace.install_dialect<Dialect>("Library"_view));

    auto interpreted = workspace.interpret_source(
        errors, "Rejected"_view, rejection.path, *source);
    Bool completed =
        interpreted && workspace.link(errors) && workspace.finalize(errors);

    EXPECT_NOT(completed);
    EXPECT_EQ(errors.get_size(), Count(1));

    Allocator::Arena rendered_domain;
    View::Bytes rendered = errors.render_message(rendered_domain, 0);
    EXPECT(Algorithm::search(rendered, rejection.message) != Count(-1));
    EXPECT(Algorithm::search(rendered, rejection.source_line) != Count(-1));
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, foreign_workspace_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/foreign_triad.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "ForeignAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  auto& surface = monograph.get_source().get_foreign();

  ASSERT(workspace.link(errors));
  ASSERT(monograph.link());
  ASSERT(workspace.finalize(errors));
  ASSERT(monograph.finalize());

  auto readonly = surface.select_state("imported_readonly"_view);
  auto state = surface.select_state("imported_state"_view);
  auto function = surface.select_function("imported_function"_view);
  ASSERT(readonly);
  ASSERT(state);
  ASSERT(function);
  EXPECT(
      readonly->get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed);
  EXPECT(state->get_visibility() == Tetrodotoxin::Language::Visibility::Public);
  EXPECT(&state->get_type() == &Dialect::get_unsigned_64());
  EXPECT_EQ(function->get_parameters().get_size(), Count(2));
  EXPECT_EQ(function->get_results().get_size(), Count(1));
  EXPECT(errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(
    DialectTests,
    private_parameter_fixture_fails_publication) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/public_parameter_private_type.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "PrivateParameter"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());

  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT(&workspace.resolve_context("PrivateParameter"_view) == &monograph);
  EXPECT(errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());

  ASSERT(workspace.link(errors));
  EXPECT(errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());

  EXPECT_NOT(workspace.finalize(errors));
  EXPECT(
      &workspace.resolve_context("PrivateParameter"_view) ==
      &Invalid::get_invalid());
  ASSERT_EQ(errors.get_size(), Count(1));

  auto diagnostics = monograph.get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  const auto& diagnostic = diagnostics.get_data()[0];
  ASSERT(diagnostic.get_anchor());
  EXPECT_TEXT(
      diagnostic.get_anchor()->get_span().caculate_text(*source),
      "Hidden"_view);
  EXPECT_TEXT(
      diagnostic.get_message(),
      "Externally readable Function publishes an unreachable Type "
      "route."_view);
  EXPECT_TEXT(
      diagnostic.get_hint(),
      "Keep the Function private or publish its authored Type route."_view);
}

PERIMORTEM_UNIT_TEST(DialectTests, native_function_attributes) {
  static constexpr View::Bytes source =
      "// Native Function Attribute test.\n"
      "dialect : Library;\n"
      "@symbol(\"shared_native\")\n"
      "@abi(\"C\")\n"
      "public first : func = [] -> Bool { return true; }\n"
      "@abi(\"C\")\n"
      "@symbol(\"shared_native\")\n"
      "public second : func = [] -> Bool { return false; }\n"
      "public ordinary : func = [] -> Bool { return true; }"_view;
  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_library(workspace, errors, "Native"_view, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  auto bindings = source_type.get_callables();
  auto binding = bindings.begin();
  ASSERT(binding != bindings.end());
  ASSERT((*binding).get().is<Language::Function>());
  const auto& first = static_cast<const Language::Function&>((*binding).get());
  ++binding;
  ASSERT(binding != bindings.end());
  ASSERT((*binding).get().is<Language::Function>());
  const auto& second = static_cast<const Language::Function&>((*binding).get());
  ++binding;
  ASSERT(binding != bindings.end());
  ASSERT((*binding).get().is<Language::Function>());
  const auto& ordinary =
      static_cast<const Language::Function&>((*binding).get());
  ++binding;
  EXPECT(binding == bindings.end());
  auto first_attributes = first.get_definition().get_attributes();
  auto second_attributes = second.get_definition().get_attributes();
  ASSERT_EQ(first_attributes.get_size(), Count(2));
  ASSERT_EQ(second_attributes.get_size(), Count(2));
  EXPECT_TEXT(first_attributes.get_data()[0].get_key(), "symbol"_view);
  EXPECT_TEXT(first_attributes.get_data()[1].get_key(), "abi"_view);
  EXPECT_TEXT(second_attributes.get_data()[0].get_key(), "abi"_view);
  EXPECT_TEXT(second_attributes.get_data()[1].get_key(), "symbol"_view);
  const View::Bytes* first_symbol =
      first_attributes.get_data()[0].get_value().find<View::Bytes>();
  const View::Bytes* second_symbol =
      second_attributes.get_data()[1].get_value().find<View::Bytes>();
  ASSERT(first_symbol);
  ASSERT(second_symbol);
  EXPECT_TEXT(*first_symbol, "shared_native"_view);
  EXPECT_TEXT(*second_symbol, "shared_native"_view);
  EXPECT(ordinary.get_definition().get_attributes().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, source_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/source_acceptance.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "SourceAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(0),
      "Library source acceptance."_view);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("SourceAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  EXPECT_TEXT(source_type.get_name(), "<source>"_view);
  EXPECT(&source_type.get_definition().get_host() == &monograph);
  EXPECT_NOT(source_type.get_authorship());
  const Anchor source_anchor = source_type.get_anchor();
  EXPECT_TEXT(source_anchor.get_token().caculate_text(*source), "dialect"_view);
  EXPECT_TEXT(
      source_anchor.get_span().caculate_text(*source),
      "// Library source acceptance.\ndialect : Library;"_view);
  const Abstract& count_identity =
      source_type.resolve_context("CountAlias"_view);
  const Abstract& mode_identity = source_type.resolve_context("Mode"_view);
  const Abstract& packet_identity = source_type.resolve_context("Packet"_view);
  const Abstract& session_identity =
      source_type.resolve_context("Session"_view);
  ASSERT(count_identity.is<Alias>());
  ASSERT(mode_identity.is<Language::Types::Enumeration>());
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(session_identity.is<Language::Types::Object>());
  const auto& count_alias = static_cast<const Alias&>(count_identity);
  const auto& mode =
      static_cast<const Language::Types::Enumeration&>(mode_identity);
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const auto& session =
      static_cast<const Language::Types::Object&>(session_identity);

  auto callables = source_type.get_callables();
  auto callable = callables.begin();
  ASSERT(callable != callables.end());
  ASSERT((*callable).get().is<Language::Function>());
  const auto& exported =
      static_cast<const Language::Function&>((*callable).get());
  ++callable;
  EXPECT(callable == callables.end());
  EXPECT(&count_alias.resolve() == &Dialect::get_unsigned_64());
  ASSERT_EQ(mode.get_definition().get_attributes().get_size(), Count(1));
  EXPECT_TEXT(
      mode.get_definition().get_attributes().get_data()[0].get_key(),
      "presentation"_view);

  auto cases = mode.get_cases();
  ASSERT_EQ(cases.get_size(), Count(2));
  EXPECT_TEXT(cases.get_data()[0].get().get_name(), "idle"_view);
  EXPECT_TEXT(cases.get_data()[1].get().get_name(), "ready"_view);

  const Abstract& nested = packet.resolve_context("Nested"_view);
  ASSERT(nested.is<Language::Types::Structure>());
  EXPECT_TEXT(
      packet.get_definition().get_attributes().get_data()[0].get_key(),
      "value_type"_view);
  const auto& nested_structure =
      static_cast<const Language::Types::Structure&>(nested);
  EXPECT_TEXT(
      nested_structure.get_definition()
          .get_attributes()
          .get_data()[0]
          .get_key(),
      "nested_type"_view);
  auto packet_field_identity = packet.get_layout().get_abstract(0);
  ASSERT(packet_field_identity);
  const auto& packet_field =
      static_cast<const Language::Field&>(*packet_field_identity);
  EXPECT_TEXT(packet_field.get_name(), "nested"_view);
  EXPECT(&packet_field.get_type() == &nested);
  EXPECT_TEXT(
      packet_field.get_definition().get_attributes().get_data()[0].get_key(),
      "member"_view);

  auto id_identity = session.get_layout().get_abstract(0);
  auto ready_identity = session.get_layout().get_abstract(1);
  ASSERT(id_identity);
  ASSERT(ready_identity);
  const auto& id_field = static_cast<const Language::Field&>(*id_identity);
  const auto& ready_field =
      static_cast<const Language::Field&>(*ready_identity);
  EXPECT_TEXT(id_field.get_name(), "id"_view);
  EXPECT_TEXT(ready_field.get_name(), "ready"_view);
  EXPECT(&id_field.get_type() == &Dialect::get_unsigned_64());
  EXPECT_TEXT(
      session.get_definition().get_attributes().get_data()[0].get_key(),
      "reference_type"_view);
  EXPECT_TEXT(
      id_field.get_definition().get_attributes().get_data()[0].get_key(),
      "identity"_view);

  const Abstract& source_field_identity =
      source_type.resolve_context("session"_view);
  ASSERT(source_field_identity.is<Language::Field>());
  const auto& source_field =
      static_cast<const Language::Field&>(source_field_identity);
  auto initializer = source_field.get_initializer();
  ASSERT(initializer && initializer->is<Language::Expressions::Initializer>());
  const auto& object_initializer =
      static_cast<const Language::Expressions::Initializer&>(*initializer);
  EXPECT(&object_initializer.get_type() == &session);
  auto initializer_attributes = source_field.get_definition().get_attributes();
  ASSERT_EQ(initializer_attributes.get_size(), Count(2));
  EXPECT_TEXT(
      initializer_attributes.get_data()[0].get_key(), "initializer"_view);
  EXPECT_TEXT(initializer_attributes.get_data()[1].get_key(), "abi"_view);

  auto attributes = exported.get_definition().get_attributes();
  ASSERT_EQ(attributes.get_size(), Count(4));
  EXPECT_TEXT(attributes.get_data()[0].get_key(), "abi"_view);
  EXPECT_TEXT(attributes.get_data()[1].get_key(), "symbol"_view);
  const View::Bytes* symbol =
      attributes.get_data()[1].get_value().find<View::Bytes>();
  ASSERT(symbol);
  EXPECT_TEXT(*symbol, "source_acceptance"_view);
  EXPECT_TEXT(attributes.get_data()[2].get_key(), "tooling"_view);
  EXPECT_TEXT(attributes.get_data()[3].get_key(), "tooling"_view);
  EXPECT(errors.is_empty());
}

static auto completes_fixture(View::Bytes path) -> Bool {
  auto source = File::read(path);
  BAIL_IF(!source);
  Workspace workspace;
  Errors errors;
  BAIL_IF(!workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "CanonicalLibrary"_view, path, *source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return workspace.link(errors) && workspace.finalize(errors) &&
         errors.is_empty();
}

PERIMORTEM_UNIT_TEST(DialectTests, broad_source_completes) {
  EXPECT(completes_fixture("validation/data/ttx/library/broad.ttx"_view));
}

PERIMORTEM_UNIT_TEST(DialectTests, native_source_completes) {
  EXPECT(completes_fixture("validation/data/ttx/library/native.ttx"_view));
}

PERIMORTEM_UNIT_TEST(DialectTests, slice_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/value_acceptance.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted =
      workspace.interpret_source(errors, "ValueAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("ValueAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  const Abstract& packet_identity = source_type.resolve_context("Packet"_view);
  const Abstract& pair_identity = source_type.resolve_context("Pair"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(pair_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const auto& pair =
      static_cast<const Language::Types::Structure&>(pair_identity);
  auto width = find_field(packet, "width"_view);
  auto height = find_field(packet, "height"_view);
  ASSERT(width);
  ASSERT(height);

  auto sum = find_field(source_type, "sum"_view);
  ASSERT(sum && sum->get_initializer());
  ASSERT(sum->get_initializer()->is<Language::Operations::Add>());
  const auto& sum_expression =
      static_cast<const Language::Expression&>(*sum->get_initializer());
  auto folded_sum = sum_expression.get_folded();
  ASSERT(folded_sum && folded_sum->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_sum)
          .get_value(),
      Unsigned_64(5));

  auto constant_offset = find_field(source_type, "constant_offset"_view);
  ASSERT(constant_offset && constant_offset->get_initializer());
  const auto& constant_offset_expression =
      static_cast<const Language::Expression&>(
          *constant_offset->get_initializer());
  auto folded_offset = constant_offset_expression.get_folded();
  ASSERT(folded_offset && folded_offset->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_offset)
          .get_value(),
      Unsigned_64(13));

  auto sequence = find_field(source_type, "sequence"_view);
  auto selected_byte = find_field(source_type, "selected_byte"_view);
  auto missing_byte = find_field(source_type, "missing_byte"_view);
  auto selected_slice = find_field(source_type, "selected_slice"_view);
  auto missing_slice = find_field(source_type, "missing_slice"_view);
  auto constant_slice = find_field(source_type, "constant_slice"_view);
  ASSERT(sequence);
  ASSERT(selected_byte);
  ASSERT(missing_byte);
  ASSERT(selected_slice);
  ASSERT(missing_slice);
  ASSERT(constant_slice && constant_slice->get_initializer());
  ASSERT(sequence->get_type().is<Language::Types::Range>());
  const auto& range =
      static_cast<const Language::Types::Range&>(sequence->get_type());
  EXPECT(&range.get_element_type() == &Dialect::get_unsigned_64());
  EXPECT(&selected_byte->get_type() == &Dialect::get_unsigned_8());
  EXPECT(&missing_byte->get_type() == &Dialect::get_unsigned_8());
  ASSERT(
      missing_byte->get_initializer() &&
      missing_byte->get_initializer()->is<Language::Access::Slice>());
  const auto& default_expression = static_cast<const Language::Expression&>(
      *missing_byte->get_initializer());
  auto folded_default = default_expression.get_folded();
  ASSERT(folded_default && folded_default->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_default)
          .get_value(),
      Unsigned_64(0));
  ASSERT(selected_slice->get_type().is<Language::Types::Fixed>());
  EXPECT(&selected_slice->get_type() == &missing_slice->get_type());
  const auto& slice_type =
      static_cast<const Language::Types::Fixed&>(selected_slice->get_type());
  EXPECT(&slice_type.get_element_type() == &Dialect::get_unsigned_8());
  EXPECT_EQ(slice_type.get_extent(), Unsigned_64(2));

  const auto& constant_slice_expression =
      static_cast<const Language::Expression&>(
          *constant_slice->get_initializer());
  auto folded_constant_slice = constant_slice_expression.get_folded();
  ASSERT(folded_constant_slice);
  ASSERT_EQ(folded_constant_slice->get_layout().get_size(), Count(2));
  auto constant_slice_first =
      folded_constant_slice->get_layout().get_abstract(0);
  auto constant_slice_second =
      folded_constant_slice->get_layout().get_abstract(1);
  ASSERT(
      constant_slice_first &&
      constant_slice_first->is<Language::Constants::Unsigned>());
  ASSERT(
      constant_slice_second &&
      constant_slice_second->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*constant_slice_first)
          .get_value(),
      Unsigned_64(0x0D));
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*constant_slice_second)
          .get_value(),
      Unsigned_64(0x0E));

  auto called = find_field(source_type, "called"_view);
  auto addressed = find_field(source_type, "addressed"_view);
  auto self_called = find_field(source_type, "self_called"_view);
  ASSERT(called && called->get_initializer());
  ASSERT(addressed && addressed->get_initializer());
  ASSERT(self_called && self_called->get_initializer());
  ASSERT(called->get_initializer()->is<Language::Access::Call>());
  ASSERT(addressed->get_initializer()->is<Language::Access::Address>());
  ASSERT(self_called->get_initializer()->is<Language::Access::Call>());
  const auto& address = static_cast<const Language::Access::Address&>(
      *addressed->get_initializer());
  const auto& self_call = static_cast<const Language::Access::Call&>(
      *self_called->get_initializer());
  EXPECT(address.get_receiver().get_result().is<Addressable>());
  EXPECT(&address.get_result() == &*width);
  ASSERT(self_call.get_callable());
  EXPECT(self_call.get_callable()->is_type_bound(packet));

  auto empty = find_function(source_type, "empty"_view);
  auto single = find_field(source_type, "single"_view);
  auto reordered = find_field(source_type, "reordered"_view);
  ASSERT(empty);
  auto empty_return = find_return(*empty);
  ASSERT(empty_return);
  EXPECT_TEXT(
      empty_return->get_anchor().get_span().caculate_text(*source),
      "return packet.[];"_view);
  EXPECT(empty->get_results().is_empty());
  ASSERT(single && single->get_initializer());
  ASSERT(reordered && reordered->get_initializer());
  ASSERT(single->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(reordered->get_initializer()->is<Language::Access::Swizzle>());
  const auto& single_swizzle =
      static_cast<const Language::Access::Swizzle&>(*single->get_initializer());
  const auto& reordered_swizzle = static_cast<const Language::Access::Swizzle&>(
      *reordered->get_initializer());
  ASSERT_EQ(single_swizzle.get_layout().get_size(), Count(1));
  auto single_output = single_swizzle.get_layout().get_abstract(0);
  ASSERT(single_output && single_output->is<Language::Access::Address>());
  EXPECT(
      &static_cast<const Language::Access::Address&>(*single_output)
           .get_result() == &*width);
  ASSERT_EQ(reordered_swizzle.get_layout().get_size(), Count(2));
  auto first_output = reordered_swizzle.get_layout().get_abstract(0);
  auto second_output = reordered_swizzle.get_layout().get_abstract(1);
  ASSERT(first_output && first_output->is<Language::Access::Address>());
  ASSERT(second_output && second_output->is<Language::Access::Address>());
  EXPECT(
      &static_cast<const Language::Access::Address&>(*first_output)
           .get_result() == &*height);
  EXPECT(
      &static_cast<const Language::Access::Address&>(*second_output)
           .get_result() == &*width);
  EXPECT(reordered_swizzle.fits(pair));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, executable_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/executable_acceptance.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "ExecutableAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("ExecutableAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  auto execute = find_function(source_type, "execute"_view);
  ASSERT(execute && execute->get_body());
  const auto& parameters = execute->get_parameters();
  auto flag = parameters.get_abstract(0);
  ASSERT(flag);

  // Block order is the executable source order. Keeping the complete owner
  // inventory flat here also proves the Call remains the statement itself.
  auto statements = execute->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(7));
  ASSERT(statements.get_data()[0].get().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[2].get().is<Language::Access::Call>());
  ASSERT(statements.get_data()[3].get().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[4].get().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[5].get().is<Language::Flow::RangeLoop>());
  ASSERT(statements.get_data()[6].get().is<Language::Flow::Match>());

  const auto& total =
      static_cast<const Language::Flow::Local&>(statements.get_data()[0].get());
  const auto& one =
      static_cast<const Language::Flow::Local&>(statements.get_data()[1].get());
  EXPECT_TEXT(total.get_name(), "total"_view);
  EXPECT_TEXT(one.get_name(), "one"_view);
  EXPECT(total.get_writability() == Language::Writability::Full);
  EXPECT(one.get_writability() == Language::Writability::Constant);

  const auto& call = static_cast<const Language::Access::Call&>(
      statements.get_data()[2].get());
  ASSERT(call.get_callable());
  EXPECT_TEXT(call.get_callable()->get_name(), "tick"_view);
  EXPECT_NOT(call.get_folded());

  const auto& conditional = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[3].get());
  EXPECT(conditional.get_kind() == Language::Flow::Branch::Kind::If);
  ASSERT_EQ(conditional.get_body().get_statements().get_size(), Count(1));
  ASSERT(conditional.get_body()
             .get_statements()
             .get_data()[0]
             .get()
             .is<Language::Flow::Assignment>());
  ASSERT(conditional.get_alternate());
  ASSERT_EQ(conditional.get_alternate()->get_statements().get_size(), Count(1));
  ASSERT(conditional.get_alternate()
             ->get_statements()
             .get_data()[0]
             .get()
             .is<Language::Flow::Assignment>());

  const auto& while_loop = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[4].get());
  EXPECT(while_loop.get_kind() == Language::Flow::Branch::Kind::While);
  auto while_statements = while_loop.get_body().get_statements();
  ASSERT_EQ(while_statements.get_size(), Count(2));
  ASSERT(while_statements.get_data()[0].get().is<Language::Flow::Assignment>());
  const auto& broken = static_cast<const Language::Flow::LoopControl&>(
      while_statements.get_data()[1].get());
  EXPECT(broken.get_kind() == Language::Flow::LoopControl::Kind::Break);
  EXPECT(&broken.get_target() == &while_loop);

  // The nested Branch contributes lexical scope but does not replace the
  // RangeLoop selected by continue. The retained edge stays on the real loop.
  const auto& range_loop = static_cast<const Language::Flow::RangeLoop&>(
      statements.get_data()[5].get());
  auto range_statements = range_loop.get_body().get_statements();
  ASSERT_EQ(range_statements.get_size(), Count(2));
  const auto& range_branch = static_cast<const Language::Flow::Branch&>(
      range_statements.get_data()[0].get());
  const auto& continued = static_cast<const Language::Flow::LoopControl&>(
      range_branch.get_body().get_statements().get_data()[0].get());
  EXPECT(continued.get_kind() == Language::Flow::LoopControl::Kind::Continue);
  EXPECT(&continued.get_target() == &range_loop);
  ASSERT(range_statements.get_data()[1].get().is<Language::Flow::Assignment>());

  // Exhaustive Flag cases make Match the Function terminal. Each Return stays
  // inside its real case Block rather than becoming a copied result edge.
  const auto& match =
      static_cast<const Language::Flow::Match&>(statements.get_data()[6].get());
  EXPECT(&match.get_input().get_result() == &*flag);
  ASSERT_EQ(match.get_case_count(), Count(2));
  auto first_case = match.get_case_constant(0);
  auto second_case = match.get_case_constant(1);
  ASSERT(first_case && first_case->is<Language::Constants::Flag>());
  ASSERT(second_case && second_case->is<Language::Constants::Flag>());
  EXPECT_NOT(
      static_cast<const Language::Constants::Flag&>(*first_case).get_value());
  EXPECT(
      static_cast<const Language::Constants::Flag&>(*second_case).get_value());
  ASSERT(match.get_case_body(0));
  ASSERT(match.get_case_body(1));
  ASSERT(match.get_case_body(0)
             ->get_statements()
             .get_data()[0]
             .get()
             .is<Language::Flow::Return>());
  ASSERT(match.get_case_body(1)
             ->get_statements()
             .get_data()[0]
             .get()
             .is<Language::Flow::Return>());
  EXPECT_NOT(match.get_default());
  EXPECT_NOT(match.reaches_next_statement());
  EXPECT_NOT(execute->get_body()->reaches_next_statement());

  const Abstract& retained_match = match;
  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  EXPECT(
      &execute->get_body()->get_statements().get_data()[6].get() ==
      &retained_match);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, resource_slice_folds_const_access) {
  static constexpr View::Bytes source =
      "public const offset : Unsigned_64 = 1;\n"
      "public const size : Unsigned_64 = 2;\n"
      "private folded : Fixed[Unsigned_8, 2] =\n"
      "  $[resource/hello.txt]:[\n"
      "    source.offset + 12,\n"
      "    source.size\n"
      "  ];"_view;
  Allocator::Arena arena;
  ResourceRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "resource-slice-fold.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  auto folded = find_field(monograph.get_source(), "folded"_view);
  ASSERT(folded && folded->get_initializer());
  const auto& slice =
      static_cast<const Language::Expression&>(*folded->get_initializer());
  auto constants = slice.get_folded();
  ASSERT(constants);
  ASSERT_EQ(constants->get_layout().get_size(), Count(2));
  auto first = constants->get_layout().get_abstract(0);
  auto second = constants->get_layout().get_abstract(1);
  ASSERT(first && first->is<Language::Constants::Unsigned>());
  ASSERT(second && second->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*first).get_value(),
      Unsigned_64('D'));
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*second).get_value(),
      Unsigned_64('E'));
  EXPECT(errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, const_field_access_is_type_owned) {
  static constexpr View::Bytes source =
      "public Packet : struct {\n"
      "  public const offset := base;\n"
      "  public const base : Unsigned_64 = 1;\n"
      "  public state value : Unsigned_64;\n"
      "}\n"
      "private packet : Packet;\n"
      "private from_type := Packet.offset + 12;\n"
      "private from_address := packet.offset + 12;"_view;
  Allocator::Arena arena;
  ResourceRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "instance-const-fold.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  ASSERT(monograph.link());
  const Abstract& packet_identity = monograph.resolve_context("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  ASSERT_EQ(packet.get_layout().get_size(), Count(1));
  auto instance_entry = packet.get_layout().get_abstract(0);
  ASSERT(instance_entry);
  EXPECT_TEXT(instance_entry->get_name(), "value"_view);
  auto offset = find_field(packet, "offset"_view);
  ASSERT(offset);
  auto linked_constant = offset->get_constant();
  ASSERT(linked_constant);
  ASSERT(linked_constant->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*linked_constant)
          .get_value(),
      Unsigned_64(1));

  ASSERT(monograph.finalize());

  auto from_type = find_field(monograph.get_source(), "from_type"_view);
  auto from_address = find_field(monograph.get_source(), "from_address"_view);
  ASSERT(from_type && from_type->get_initializer());
  ASSERT(from_address && from_address->get_initializer());
  auto type_expression =
      from_type->get_initializer()->select<Language::Expression>();
  auto address_expression =
      from_address->get_initializer()->select<Language::Expression>();
  ASSERT(type_expression);
  ASSERT(address_expression);
  auto type_constant = type_expression->get_folded();
  auto address_constant = address_expression->get_folded();
  ASSERT(type_constant && type_constant->is<Language::Constants::Unsigned>());
  ASSERT(
      address_constant &&
      address_constant->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*type_constant)
          .get_value(),
      Unsigned_64(13));
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*address_constant)
          .get_value(),
      Unsigned_64(13));
  EXPECT(errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, native_function_attribute_misuse) {
  static constexpr Static::Vector<View::Bytes, 7> rejected = {{
    "// Duplicate Attribute.\n"
    "dialect : Library;\n"
    "@abi(\"C\") @abi(\"C\") public invalid : func = [] -> Bool { return true; }"_view,
    "// Unsupported ABI.\n"
    "dialect : Library;\n"
    "@abi(\"Rust\") public invalid : func = [] -> Bool { return true; }"_view,
    "// Wrong ABI carrier.\n"
    "dialect : Library;\n"
    "@abi(1) public invalid : func = [] -> Bool { return true; }"_view,
    "// Missing ABI.\n"
    "dialect : Library;\n"
    "@symbol(\"invalid\") public invalid : func = [] -> Bool { return true; }"_view,
    "// Empty symbol.\n"
    "dialect : Library;\n"
    "@abi(\"C\") @symbol(\"\") public invalid : func = [] -> Bool { return true; }"_view,
    "// Private export.\n"
    "dialect : Library;\n"
    "@abi(\"C\") private invalid : func = [] -> Bool { return true; }"_view,
    "// Self export.\n"
    "dialect : Library;\n"
    "public Host : object {\n"
    "  @abi(\"C\") public invalid : func = [self] -> Bool { return true; }\n"
    "}"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_library_source(rejected[i]));
  }
}
