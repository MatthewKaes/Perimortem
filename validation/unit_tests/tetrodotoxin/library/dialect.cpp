// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/initializer.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
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

PERIMORTEM_UNIT_TEST(DialectTests, callable_candidates_preserve_order) {
  static constexpr View::Bytes source =
      "// Overloaded Library source.\n"
      "dialect : Library;\n"
      "public repeated : func = [] -> Void {}\n"
      "public repeated : func = [Bool] -> Void {}"_view;
  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_library(workspace, errors, "Overloaded"_view, source);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  auto candidates =
      source_type.get_callables(Tetrodotoxin::Language::Visibility::Public);
  auto candidate = candidates.begin();
  ASSERT(candidate != candidates.end());
  ASSERT((*candidate).get().is<Language::Function>());
  const auto& first =
      static_cast<const Language::Function&>((*candidate).get());
  ++candidate;
  ASSERT(candidate != candidates.end());
  ASSERT((*candidate).get().is<Language::Function>());
  const auto& second =
      static_cast<const Language::Function&>((*candidate).get());
  ++candidate;
  EXPECT(candidate == candidates.end());
  EXPECT_EQ(first.get_parameters().get_size(), Count(0));
  EXPECT_EQ(second.get_parameters().get_size(), Count(1));
  EXPECT(
      &monograph->resolve_context("repeated"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
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
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "public broken : Missing;\npublic later : func = [] -> Void {}"_view,
    "public broken : Bool = absent;\npublic later : func = [] -> Void {}"_view,
    "public const broken : Bool = 1;\n"
    "public later : func = [] -> Void {}"_view,
    "public const broken : Unsigned_8 = 256;\n"
    "public later : func = [] -> Void {}"_view,
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

PERIMORTEM_UNIT_TEST(DialectTests, top_level_self_fails_signature_linking) {
  static constexpr View::Bytes source =
      "public invalid : func = [self] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "top-level-self.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  const auto& source_type = monograph.get_source();
  auto callables = source_type.get_callables();
  auto callable = callables.begin();
  ASSERT(callable != callables.end());
  ASSERT((*callable).get().is<Language::Function>());
  const auto& function =
      static_cast<const Language::Function&>((*callable).get());
  ++callable;
  EXPECT(callable == callables.end());
  EXPECT(errors.is_empty());

  ASSERT_NOT(monograph.link());
  auto receiver = function.get_parameters().get_abstract(0);
  ASSERT(receiver);
  EXPECT(receiver->visit<Addressable>(
      [&](const Addressable& parameter) {
        return Bool(
            parameter.get_name() == "self"_view &&
            &parameter.get_type() == &monograph.get_source());
      },
      [](const Abstract&) { return False; }));
  ASSERT_EQ(monograph.get_diagnostics().get_size(), Count(1));
  ASSERT(monograph.get_diagnostics().get_data()[0].get_anchor());
  EXPECT_TEXT(
      monograph.get_diagnostics()
          .get_data()[0]
          .get_anchor()
          ->get_span()
          .caculate_text(source),
      "invalid"_view);
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
  EXPECT(&public_alias.get_target() == &hidden);
  EXPECT(&private_alias.get_target() == &hidden);
  EXPECT_EQ(public_alias.get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      public_alias.get_documentation().get_line(0),
      "Local documentation."_view);
  EXPECT_TEXT(
      public_alias.get_documentation().get_line(1),
      "Hidden documentation."_view);
  EXPECT(&private_alias.get_documentation() == &hidden.get_documentation());
  EXPECT(&monograph.resolve_context("PublicAlias"_view) == &public_identity);
  EXPECT(
      &monograph.resolve_context("PrivateAlias"_view) ==
      &Invalid::get_invalid());

  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  EXPECT(&public_alias.resolve() == &hidden);
  EXPECT(&private_alias.resolve() == &hidden);
  EXPECT(&monograph.resolve_context("PublicAlias"_view) == &public_identity);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(DialectTests, rejected_source_alias_is_atomic) {
  static constexpr Static::Vector<View::Bytes, 8> rejected = {{
    "// Rejected documentation.\npublic Broken : alias Bool;"_view,
    "public Broken : alias = Missing;"_view,
    "public Broken : alias = Fact;"_view,
    "public Bool : alias = Unsigned_8;"_view,
    "public Outer : alias = Unsigned_8;"_view,
    "public Self : alias = Self;"_view,
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
    EXPECT(cursor.matches(i == 0 ? Code::Type::Comment : Code::Type::Public));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, focused_fixture_rejections) {
  struct Rejection {
    View::Bytes path;
    View::Bytes message;
    View::Bytes source_line;
  };
  static constexpr Static::Vector<Rejection, 5> rejections = {{
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
      "private state inferred := new;"_view,
    },
    {
      "validation/data/ttx/library/ordinary_bodyless.ttx"_view,
      "Library Function signatures require a body beginning with `{`."_view,
      "public missing_body : func = [] -> Unsigned_64;"_view,
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

    EXPECT_NOT(interpreted);
    EXPECT_EQ(errors.get_size(), Count(1));
    EXPECT(
        &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());

    Allocator::Arena rendered_domain;
    View::Bytes rendered = errors.render_message(rendered_domain, 0);
    EXPECT(Algorithm::search(rendered, rejection.message) != Count(-1));
    EXPECT(Algorithm::search(rendered, rejection.source_line) != Count(-1));
  }
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
  ASSERT(initializer && initializer->is<Language::Initializer>());
  const auto& object_initializer =
      static_cast<const Language::Initializer&>(*initializer);
  EXPECT(&object_initializer.get_type() == &session);
  ASSERT_EQ(object_initializer.get_inputs().get_size(), Count(1));
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
