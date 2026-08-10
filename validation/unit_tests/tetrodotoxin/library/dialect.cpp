// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/value.hpp"

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

static_assert(
    !__is_constructible(Language::Monograph, const Language::Monograph&));
static_assert(!__is_constructible(Language::Monograph, Language::Monograph&&));

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

class OuterFact : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "outer"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class OuterRegistry : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "OuterRegistry"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == fact.get_name()) {
      return fact;
    }

    return Invalid::get_invalid();
  }

  OuterFact fact;
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

class AliasAddressable : public Addressable {
 public:
  constexpr AliasAddressable(View::Bytes name, const Type& type)
      : name(name), type(type) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  constexpr auto get_type() const -> const Type& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
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

static Harness DialectTests = {
  .name = "Tetrodotoxin::Library::Dialect"_view,
};

static constexpr Static::Vector<View::Bytes, 12> intrinsic_names = {{
  "Bool"_view,
  "Unsigned_8"_view,
  "Unsigned_16"_view,
  "Unsigned_32"_view,
  "Unsigned_64"_view,
  "Signed_8"_view,
  "Signed_16"_view,
  "Signed_32"_view,
  "Signed_64"_view,
  "Real_32"_view,
  "Real_64"_view,
  "Void"_view,
}};

PERIMORTEM_UNIT_TEST(DialectTests, canonical_intrinsic_addresses) {
  EmptyRegistry registry;
  Dialect dialect;
  Static::Vector<const Abstract*, 12> addresses = {{
    &Dialect::get_bool(),
    &Dialect::get_unsigned_8(),
    &Dialect::get_unsigned_16(),
    &Dialect::get_unsigned_32(),
    &Dialect::get_unsigned_64(),
    &Dialect::get_signed_8(),
    &Dialect::get_signed_16(),
    &Dialect::get_signed_32(),
    &Dialect::get_signed_64(),
    &Dialect::get_real_32(),
    &Dialect::get_real_64(),
    &Dialect::get_void(),
  }};

  for (Count i = 0; i < intrinsic_names.get_size(); i++) {
    EXPECT(
        &dialect.resolve_intrinsic(intrinsic_names[i]).resolve() ==
        addresses[i]);
  }
  EXPECT(Dialect::get_bool().is<Ttx::Model::Types::Flag>());
  EXPECT(Dialect::get_unsigned_8().is<Ttx::Model::Types::Unsigned>());
  EXPECT(Dialect::get_signed_8().is<Ttx::Model::Types::Signed>());
  EXPECT(Dialect::get_real_32().is<Ttx::Model::Types::Real>());
  EXPECT(Dialect::get_void().is<Type>());
}

PERIMORTEM_UNIT_TEST(DialectTests, declaration_graph) {
  static constexpr View::Bytes first_source =
      "// First Library source.\n"
      "dialect : Library;\n"
      "// Alpha documentation.\n"
      "public func alpha[Bool] -> Void {}\n"
      "// Hidden documentation.\n"
      "private func hidden[] -> Signed_32 {}\n"
      "// Beta documentation.\n"
      "public func beta[.value : Unsigned_16] -> [.ready : Bool] {}"_view;
  static constexpr View::Bytes second_source =
      "// Reversed Library source.\n"
      "dialect : Library;\n"
      "public func beta[.value : Unsigned_16] -> [.ready : Bool] {}\n"
      "public func alpha[Bool] -> Void {}\n"
      "private func hidden[] -> Signed_32 {}"_view;
  Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto first = import_library(workspace, errors, "First"_view, first_source);
  auto second = import_library(workspace, errors, "Second"_view, second_source);
  ASSERT(first && second);
  EXPECT(errors.is_empty());
  EXPECT_TEXT(first->get_name(), "Library"_view);
  EXPECT_TEXT(
      first->get_documentation().get_line(0), "First Library source."_view);

  ASSERT(first->get_source().is<Language::Types::Source>());
  ASSERT(second->get_source().is<Language::Types::Source>());
  const auto& first_scope =
      static_cast<const Language::Types::Source&>(first->get_source());
  const auto& second_scope =
      static_cast<const Language::Types::Source&>(second->get_source());
  EXPECT(&first_scope.get_documentation() == &first->get_documentation());
  EXPECT(&second_scope.get_documentation() == &second->get_documentation());
  EXPECT_TEXT(
      first_scope.get_documentation().get_line(0),
      "First Library source."_view);
  EXPECT(&first->resolve_context("source"_view) == &first_scope);
  EXPECT(&second->resolve_context("source"_view) == &second_scope);

  // Context lookup excludes Callables because only invocation can select one.
  // The source candidate views preserve authored order and visibility without
  // changing any Function identity.
  auto first_bindings = first->get_authored_bindings();
  auto second_bindings = second->get_authored_bindings();
  ASSERT_EQ(first_bindings.get_size(), Count(3));
  ASSERT_EQ(second_bindings.get_size(), Count(3));
  const Abstract& first_alpha = first_bindings.get_data()[0].get();
  const Abstract& first_hidden = first_bindings.get_data()[1].get();
  const Abstract& first_beta = first_bindings.get_data()[2].get();
  const Abstract& second_beta = second_bindings.get_data()[0].get();
  const Abstract& second_alpha = second_bindings.get_data()[1].get();
  const Abstract& second_hidden = second_bindings.get_data()[2].get();
  ASSERT(
      first_alpha.is<Language::Function>() &&
      first_hidden.is<Language::Function>() &&
      first_beta.is<Language::Function>() &&
      second_alpha.is<Language::Function>() &&
      second_hidden.is<Language::Function>() &&
      second_beta.is<Language::Function>());
  EXPECT(&first->resolve_context("alpha"_view) == &Invalid::get_invalid());
  EXPECT(&first->resolve_context("beta"_view) == &Invalid::get_invalid());
  EXPECT(&first->resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&second->resolve_context("alpha"_view) == &Invalid::get_invalid());
  EXPECT(&second->resolve_context("beta"_view) == &Invalid::get_invalid());
  EXPECT(&second->resolve_context("hidden"_view) == &Invalid::get_invalid());

  auto first_public = first_scope.get_callable_bindings();
  auto second_public = second_scope.get_callable_bindings();
  auto first_functions = first_scope.get_callable_bindings(*first);
  auto second_functions = second_scope.get_callable_bindings(*second);
  ASSERT_EQ(first_public.get_size(), Count(2));
  ASSERT_EQ(second_public.get_size(), Count(2));
  ASSERT_EQ(first_functions.get_size(), Count(3));
  ASSERT_EQ(second_functions.get_size(), Count(3));
  EXPECT(&first_public.get_data()[0].get() == &first_alpha);
  EXPECT(&first_public.get_data()[1].get() == &first_beta);
  EXPECT(&second_public.get_data()[0].get() == &second_beta);
  EXPECT(&second_public.get_data()[1].get() == &second_alpha);
  EXPECT(&first_functions.get_data()[0].get() == &first_alpha);
  EXPECT(&first_functions.get_data()[1].get() == &first_hidden);
  EXPECT(&first_functions.get_data()[2].get() == &first_beta);
  EXPECT(&second_functions.get_data()[0].get() == &second_beta);
  EXPECT(&second_functions.get_data()[1].get() == &second_alpha);
  EXPECT(&second_functions.get_data()[2].get() == &second_hidden);
  EXPECT(first_hidden.get_name() == "hidden"_view);
  EXPECT(second_hidden.get_name() == "hidden"_view);
  EXPECT(
      static_cast<const Language::Function&>(first_alpha).get_visibility() ==
      Language::Visibility::Public);
  EXPECT(
      static_cast<const Language::Function&>(first_hidden).get_visibility() ==
      Language::Visibility::Private);
  EXPECT_TEXT(
      first_alpha.get_documentation().get_line(0), "Alpha documentation."_view);
  EXPECT_TEXT(
      first_hidden.get_documentation().get_line(0),
      "Hidden documentation."_view);
  EXPECT(&first_alpha.resolve() == &first_alpha);
  EXPECT(&first_beta.resolve() == &first_beta);
  const auto& alpha = static_cast<const Language::Function&>(first_alpha);
  EXPECT(&alpha.get_source() == &*first);
  EXPECT(&alpha.get_host() == &first_scope);

  // Hosted Functions use the same installed Dialect Types through their source
  // Structure. The Monograph itself exposes no ambient or intrinsic names.
  for (Count i = 0; i < intrinsic_names.get_size(); i++) {
    const Abstract& first_intrinsic = alpha.resolve_context(intrinsic_names[i]);
    const Abstract& second_intrinsic =
        static_cast<const Language::Function&>(second_alpha)
            .resolve_context(intrinsic_names[i]);
    EXPECT(&first_intrinsic != &Invalid::get_invalid());
    EXPECT(&first_intrinsic == &second_intrinsic);
    EXPECT(first_intrinsic.is<Type>());
    EXPECT_TEXT(first_intrinsic.get_name(), intrinsic_names[i]);
    EXPECT(
        &first->resolve_context(intrinsic_names[i]) == &Invalid::get_invalid());
  }

  const Abstract& boolean = alpha.resolve_context("Bool"_view);
  const Abstract& unsigned_16 = alpha.resolve_context("Unsigned_16"_view);
  const Abstract& void_type = alpha.resolve_context("Void"_view);
  ASSERT(void_type.is<Type>());
  EXPECT_NOT(void_type.is<Types::Value>());
  EXPECT(static_cast<const Type&>(void_type).get_layout().is_empty());
  EXPECT_NOT(void_type.get_documentation().is_empty());
  EXPECT(
      &void_type.resolve_context("anything"_view) == &Invalid::get_invalid());

  // Layout edges reach Dialect identities directly. Named inputs become real
  // Parameter Addressables while named results retain their passive Alias.
  ASSERT_EQ(alpha.get_parameters().get_size(), Count(1));
  ASSERT_EQ(alpha.get_results().get_size(), Count(1));
  EXPECT(alpha.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& edge) { return &edge == &boolean ? True : False; }));
  EXPECT(alpha.get_results().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& edge) {
        return &edge == &void_type ? True : False;
      }));

  const auto& beta = static_cast<const Language::Function&>(first_beta);
  ASSERT_EQ(beta.get_parameters().get_size(), Count(1));
  ASSERT_EQ(beta.get_results().get_size(), Count(1));
  EXPECT(beta.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& edge) {
        return edge.is<Language::Parameter>() &&
                       edge.get_name() == "value"_view &&
                       &static_cast<const Language::Parameter&>(edge)
                               .get_type() == &unsigned_16
                   ? True
                   : False;
      }));
  EXPECT(beta.get_results().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& edge) {
        return edge.is<Alias>() && edge.get_name() == "ready"_view &&
                       &edge.resolve() == &boolean
                   ? True
                   : False;
      }));

  // The external source surface admits no ambient namespaces or undeclared
  // names. Future import spellings remain ordinary misses until linking.
  static constexpr Static::Vector<View::Bytes, 6> absent = {{
    "missing"_view,
    "Count"_view,
    "Core"_view,
    "Core::Bool"_view,
    "Memory"_view,
    "Package"_view,
  }};
  for (Count i = 0; i < absent.get_size(); i++) {
    EXPECT(&first->resolve_context(absent[i]) == &Invalid::get_invalid());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, finalization_caches_function_roots) {
  static constexpr View::Bytes source =
      "// Fold finalization.\n"
      "dialect : Library;\n"
      "public func folded[] -> Unsigned_64 { 6 / 2; 1 / 0; }"_view;
  Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_library(workspace, errors, "Folded"_view, source);
  ASSERT(monograph);
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Function>());
  const auto& function =
      static_cast<const Language::Function&>(bindings.get_data()[0].get());
  auto expressions = function.get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(2));

  const Language::Expression& first = expressions.get_data()[0].get();
  const Language::Expression& second = expressions.get_data()[1].get();
  auto folded = first.get_folded();
  ASSERT(folded);
  EXPECT(folded->visit<Language::Constants::Unsigned>(
      [](const Language::Constants::Unsigned& value) {
        return value.get_value() == 3 ? True : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT_NOT(second.get_folded());
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, workspace_materializations_follow_graph) {
  static constexpr View::Bytes first_source =
      "// First literal source.\n"
      "dialect : Library;\n"
      "public func first[] -> Void { \"one\"; }"_view;
  static constexpr View::Bytes second_source =
      "// Second literal source.\n"
      "dialect : Library;\n"
      "public func second[] -> Void { \"two\"; }"_view;
  Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto first =
      import_library(workspace, errors, "FirstLiteral"_view, first_source);
  auto second =
      import_library(workspace, errors, "SecondLiteral"_view, second_source);
  ASSERT(first && second);

  // One installed Dialect keeps one materialization inventory beside every
  // Monograph graph in its Workspace Arena. Equal literal formulas therefore
  // share the exact generated Type without merging their Expression roots.
  EXPECT(&first->get_materializations() == &second->get_materializations());
  EXPECT_EQ(first->get_materializations().get_size(), Count(1));
  auto first_functions = first->get_authored_bindings();
  auto second_functions = second->get_authored_bindings();
  ASSERT_EQ(first_functions.get_size(), Count(1));
  ASSERT_EQ(second_functions.get_size(), Count(1));
  ASSERT(first_functions.get_data()[0].get().is<Language::Function>());
  ASSERT(second_functions.get_data()[0].get().is<Language::Function>());
  auto first_expressions = static_cast<const Language::Function&>(
                               first_functions.get_data()[0].get())
                               .get_expressions();
  auto second_expressions = static_cast<const Language::Function&>(
                                second_functions.get_data()[0].get())
                                .get_expressions();
  ASSERT_EQ(first_expressions.get_size(), Count(1));
  ASSERT_EQ(second_expressions.get_size(), Count(1));
  const Language::Expression& first_expression =
      first_expressions.get_data()[0].get();
  const Language::Expression& second_expression =
      second_expressions.get_data()[0].get();
  const auto& first_anchor = first_expression.get_anchor();
  const auto& second_anchor = second_expression.get_anchor();
  ASSERT(first_anchor);
  ASSERT(second_anchor);
  EXPECT(&first_expression != &second_expression);
  EXPECT(&first_expression.get_type() == &second_expression.get_type());
  EXPECT_TEXT(
      first_anchor->get_span().caculate_text(first_source), "\"one\""_view);
  EXPECT_TEXT(
      second_anchor->get_span().caculate_text(second_source), "\"two\""_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, workspace_intrinsic_sharing) {
  static constexpr View::Bytes source =
      "// Standalone Library source.\n"
      "dialect : Library;\n"
      "public func ready[] -> Void {}"_view;
  Workspace first_workspace;
  Workspace second_workspace;
  Errors first_errors;
  Errors second_errors;

  ASSERT(first_workspace.install_dialect<Dialect>("Library"_view));
  ASSERT(second_workspace.install_dialect<Dialect>("Library"_view));
  auto first =
      import_library(first_workspace, first_errors, "First"_view, source);
  auto second =
      import_library(second_workspace, second_errors, "Second"_view, source);
  ASSERT(first && second);

  auto first_bindings = first->get_authored_bindings();
  auto second_bindings = second->get_authored_bindings();
  ASSERT_EQ(first_bindings.get_size(), Count(1));
  ASSERT_EQ(second_bindings.get_size(), Count(1));
  ASSERT(first_bindings.get_data()[0].get().is<Language::Function>());
  ASSERT(second_bindings.get_data()[0].get().is<Language::Function>());
  const auto& first_function = static_cast<const Language::Function&>(
      first_bindings.get_data()[0].get());
  const auto& second_function = static_cast<const Language::Function&>(
      second_bindings.get_data()[0].get());

  // Workspaces keep independent stateful Dialects while hosted Functions see
  // one immutable intrinsic identity across every semantic island.
  for (Count i = 0; i < intrinsic_names.get_size(); i++) {
    EXPECT(
        &first_function.resolve_context(intrinsic_names[i]) ==
        &second_function.resolve_context(intrinsic_names[i]));
  }
  EXPECT(first_errors.is_empty());
  EXPECT(second_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, context_fallback_and_shadowing) {
  Allocator::Arena arena;
  OuterRegistry registry;
  Dialect dialect;
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, registry, materializations);
  const Type& source = monograph.get_source();

  // External Monograph lookup stops at the Source. Hosted Functions
  // retain the complete source, outer context, and intrinsic lookup chain.
  EXPECT(&monograph.resolve_context("source"_view) == &source);
  EXPECT(&monograph.resolve_context("outer"_view) == &Invalid::get_invalid());
  EXPECT(&monograph.resolve_context("Bool"_view) == &Invalid::get_invalid());

  Errors local_errors;
  Tokenizer local_tokenizer(
      arena, "public func local[] -> Void {}"_view, "local.ttx"_view);
  Cursor local_cursor(local_tokenizer, local_errors);
  auto local = Language::Function::reserve(
      arena, local_cursor, Documentation::get_empty(), monograph, source,
      materializations);
  ASSERT(local);
  ASSERT(monograph.bind_static(*local, local->get_visibility()));
  EXPECT(&monograph.resolve_context("local"_view) == &Invalid::get_invalid());
  ASSERT(source.is<Language::Types::Structure>());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(source);
  auto local_candidates = source_structure.get_callable_bindings();
  ASSERT_EQ(local_candidates.get_size(), Count(1));
  EXPECT(&local_candidates.get_data()[0].get() == &*local);
  EXPECT(&local->get_source() == &monograph);
  EXPECT(&local->get_host() == &source);
  EXPECT(&local->resolve_context("outer"_view) == &registry.fact);
  EXPECT(&local->resolve_context("Bool"_view) == &Dialect::get_bool());
  ASSERT(local->complete(local_cursor));
  ASSERT(local->link());

  Errors outer_errors;
  Tokenizer outer_tokenizer(
      arena, "public func outer[] -> Void {}"_view, "outer.ttx"_view);
  Cursor outer_cursor(outer_tokenizer, outer_errors);
  auto outer = Language::Function::reserve(
      arena, outer_cursor, Documentation::get_empty(), monograph, source,
      materializations);
  ASSERT(outer);
  ASSERT(monograph.bind_static(*outer, outer->get_visibility()));
  ASSERT(outer->complete(outer_cursor));
  ASSERT(outer->link());

  Errors parameter_errors;
  Tokenizer parameter_tokenizer(
      arena, "public func shadow[.outer : Bool] -> Void {}"_view,
      "parameter-shadow.ttx"_view);
  Cursor parameter_cursor(parameter_tokenizer, parameter_errors);
  auto shadow = Language::Function::reserve(
      arena, parameter_cursor, Documentation::get_empty(), monograph, source,
      materializations);
  ASSERT(shadow);
  ASSERT(monograph.bind_static(*shadow, shadow->get_visibility()));
  ASSERT(shadow->complete(parameter_cursor));
  ASSERT(shadow->link());
  const Abstract& shadowed = shadow->resolve_context("outer"_view);
  ASSERT(shadowed.is<Language::Parameter>());
  EXPECT(
      &static_cast<const Language::Parameter&>(shadowed).get_type() ==
      &Dialect::get_bool());
  EXPECT(&shadowed != &registry.fact);
  EXPECT(&monograph.resolve_context("outer"_view) == &Invalid::get_invalid());
  EXPECT(&monograph.resolve_context("shadow"_view) == &Invalid::get_invalid());
  auto candidates = source_structure.get_callable_bindings(monograph);
  ASSERT_EQ(candidates.get_size(), Count(3));
  EXPECT(&candidates.get_data()[0].get() == &*local);
  EXPECT(&candidates.get_data()[1].get() == &*outer);
  EXPECT(&candidates.get_data()[2].get() == &*shadow);
  EXPECT(local_errors.is_empty());
  EXPECT(outer_errors.is_empty());
  EXPECT(parameter_errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, callable_candidates_preserve_order) {
  static constexpr View::Bytes first_source =
      "public func repeated[] -> Void {}"_view;
  static constexpr View::Bytes duplicate_source =
      "public func repeated[Bool] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, registry, materializations);
  const Type& source = monograph.get_source();
  Errors first_errors;
  Tokenizer first_tokenizer(arena, first_source, "first.ttx"_view);
  Cursor first_cursor(first_tokenizer, first_errors);
  auto first = Language::Function::reserve(
      arena, first_cursor, Documentation::get_empty(), monograph, source,
      materializations);
  ASSERT(first);

  // A shared name is not enough to select a Callable. The candidate view keeps
  // both exact Functions in authored order for the invocation owner to fit.
  ASSERT(first->complete(first_cursor));
  ASSERT(monograph.bind_static(*first, first->get_visibility()));
  ASSERT(source.is<Language::Types::Structure>());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(source);

  Errors duplicate_errors;
  Tokenizer duplicate_tokenizer(arena, duplicate_source, "duplicate.ttx"_view);
  Cursor duplicate_cursor(duplicate_tokenizer, duplicate_errors);
  auto duplicate = Language::Function::reserve(
      arena, duplicate_cursor, Documentation::get_empty(), monograph, source,
      materializations);
  ASSERT(duplicate);
  ASSERT(duplicate->complete(duplicate_cursor));
  ASSERT(monograph.bind_static(*duplicate, duplicate->get_visibility()));
  EXPECT(
      &monograph.resolve_context("repeated"_view) == &Invalid::get_invalid());
  auto public_candidates = source_structure.get_callable_bindings();
  ASSERT_EQ(public_candidates.get_size(), Count(2));
  EXPECT(&public_candidates.get_data()[0].get() == &*first);
  EXPECT(&public_candidates.get_data()[1].get() == &*duplicate);

  // Workspace completion preserves the same candidate ordering after all
  // source barriers settle.
  static constexpr View::Bytes authored_duplicate =
      "// Overloaded Library source.\n"
      "dialect : Library;\n"
      "public func repeated[] -> Void {}\n"
      "public func repeated[Bool] -> Void {}"_view;
  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto overloaded =
      import_library(workspace, errors, "Overloaded"_view, authored_duplicate);
  ASSERT(overloaded);
  ASSERT(overloaded->get_source().is<Language::Types::Structure>());
  const auto& overloaded_source =
      static_cast<const Language::Types::Structure&>(overloaded->get_source());
  auto overloaded_authored = overloaded->get_authored_bindings();
  auto overloaded_candidates = overloaded_source.get_callable_bindings();
  ASSERT_EQ(overloaded_authored.get_size(), Count(2));
  ASSERT_EQ(overloaded_candidates.get_size(), Count(2));
  EXPECT(
      &overloaded_candidates.get_data()[0].get() ==
      &overloaded_authored.get_data()[0].get());
  EXPECT(
      &overloaded_candidates.get_data()[1].get() ==
      &overloaded_authored.get_data()[1].get());
  EXPECT(
      &overloaded->resolve_context("repeated"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, failed_field_phase_stops_root_linking) {
  static constexpr View::Bytes source =
      "public Packet : struct { public missing : Missing; }\n"
      "public func later[] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "phase-cascade.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  auto bindings = monograph.get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(2));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[1].get().is<Language::Function>());
  const auto& later =
      static_cast<const Language::Function&>(bindings.get_data()[1].get());
  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_structure =
      static_cast<const Language::Types::Source&>(monograph.get_source());

  // Field linking owns the first failing global phase. Root signatures stay
  // open so a later phase cannot seal source lookup after that rejection.
  ASSERT_NOT(monograph.link());
  EXPECT_NOT(later.is_signature_linked());
  FutureType future;
  EXPECT(source_structure.can_bind_static(future));
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

PERIMORTEM_UNIT_TEST(DialectTests, source_fields_are_static_addressables) {
  static constexpr View::Bytes source =
      "// Source Field graph.\n"
      "dialect : Library;\n"
      "// Open documentation.\n"
      "public open : Bool;\n"
      "public later : Later = seed;\n"
      "private seed : Later;\n"
      "expose state progress : Unsigned_8 = 255;\n"
      "private state hidden : Bool = false;\n"
      "public const fixed : Unsigned_16 = 7;\n"
      "public Later : struct {}\n"
      "public func open[] -> Void { open; hidden; later; }"_view;
  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_library(workspace, errors, "Fields"_view, source);
  ASSERT(monograph);
  ASSERT(monograph->get_source().is<Language::Types::Structure>());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(monograph->get_source());
  auto fields = source_structure.get_fields();
  auto public_fields = source_structure.get_public_fields();
  ASSERT_EQ(fields.get_size(), Count(6));
  ASSERT_EQ(public_fields.get_size(), Count(4));

  const Language::Field& open = fields.get_data()[0].get();
  const Language::Field& later = fields.get_data()[1].get();
  const Language::Field& seed = fields.get_data()[2].get();
  const Language::Field& progress = fields.get_data()[3].get();
  const Language::Field& hidden = fields.get_data()[4].get();
  const Language::Field& fixed = fields.get_data()[5].get();

  // One authored inventory preserves every policy and source fact while its
  // public view borrows the same identities selected below.
  EXPECT_TEXT(open.get_documentation().get_line(0), "Open documentation."_view);
  EXPECT_TEXT(
      open.get_anchor().get_span().caculate_text(source),
      "public open : Bool;"_view);
  EXPECT(open.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(open.get_writability() == Language::Field::Writability::Full);
  EXPECT(seed.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(seed.get_writability() == Language::Field::Writability::Full);
  EXPECT(progress.get_exposure() == Language::Field::Exposure::Exposed);
  EXPECT(progress.get_writability() == Language::Field::Writability::Internal);
  EXPECT(hidden.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(hidden.get_writability() == Language::Field::Writability::Internal);
  EXPECT(fixed.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(fixed.get_writability() == Language::Field::Writability::Init);

  // Later declarations satisfy both the Type route and Identifier route. The
  // narrow constant keeps its wide value while fitting the declared domain.
  const Abstract& later_type = monograph->resolve_context("Later"_view);
  EXPECT(&seed.get_type() == &later_type);
  EXPECT(&later.get_type() == &later_type);
  auto later_initializer = later.get_initializer();
  ASSERT(later_initializer && later_initializer->is<Language::Identifier>());
  auto seed_address =
      static_cast<const Language::Identifier&>(*later_initializer)
          .get_addressable();
  ASSERT(seed_address);
  EXPECT(&*seed_address == &seed);
  auto progress_initializer = progress.get_initializer();
  ASSERT(
      progress_initializer &&
      progress_initializer->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*progress_initializer)
          .get_value(),
      Unsigned_64(255));

  // Static publication follows exposure without changing source instance
  // shape or granting ordinary lookup to either private Field.
  EXPECT(&monograph->resolve_context("open"_view) == &open);
  EXPECT(&monograph->resolve_context("seed"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("later"_view) == &later);
  EXPECT(&monograph->resolve_context("progress"_view) == &progress);
  EXPECT(&monograph->resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("fixed"_view) == &fixed);
  EXPECT(source_structure.get_layout().is_empty());
  EXPECT(&open.get_host() == &source_structure);
  EXPECT(&hidden.get_host() == &source_structure);
  auto static_bindings = source_structure.get_static_bindings();
  auto external_bindings = source_structure.get_external_static_bindings();
  ASSERT_EQ(static_bindings.get_size(), Count(8));
  ASSERT_EQ(external_bindings.get_size(), Count(6));
  EXPECT(&static_bindings.get_data()[2].get() == &open);
  EXPECT(&static_bindings.get_data()[3].get() == &later);
  EXPECT(&static_bindings.get_data()[4].get() == &seed);
  EXPECT(&static_bindings.get_data()[5].get() == &progress);
  EXPECT(&static_bindings.get_data()[6].get() == &hidden);
  EXPECT(&static_bindings.get_data()[7].get() == &fixed);

  // A root Function has source authority rather than an implicit receiver.
  // Each bare Identifier therefore reaches one exact source Addressable.
  auto authored = monograph->get_authored_bindings();
  const Language::Function* root = nullptr;
  for (Count i = 0; i < authored.get_size(); i++) {
    authored.get_data()[i].get().visit<Language::Function>(
        [&](const Language::Function& function) {
          if (function.get_name() == "open"_view) {
            root = &function;
          }
        },
        [](const Abstract&) {});
  }
  ASSERT(root);
  auto expressions = root->get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(3));
  for (Count i = 0; i < expressions.get_size(); i++) {
    ASSERT(expressions.get_data()[i].get().is<Language::Identifier>());
  }
  auto root_open =
      static_cast<const Language::Identifier&>(expressions.get_data()[0].get())
          .get_addressable();
  auto root_hidden =
      static_cast<const Language::Identifier&>(expressions.get_data()[1].get())
          .get_addressable();
  auto root_later =
      static_cast<const Language::Identifier&>(expressions.get_data()[2].get())
          .get_addressable();
  ASSERT(root_open && root_hidden && root_later);
  EXPECT(&*root_open == &open);
  EXPECT(&*root_hidden == &hidden);
  EXPECT(&*root_later == &later);

  const Language::Field* first_identity = &open;
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  EXPECT(&source_structure.get_fields().get_data()[0].get() == first_identity);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, source_field_failures_stop_root_barriers) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "public broken : Missing;\npublic func later[] -> Void {}"_view,
    "public broken : Bool = absent;\npublic func later[] -> Void {}"_view,
    "public const broken : Bool = 1;\npublic func later[] -> Void {}"_view,
    "public const broken : Unsigned_8 = 256;\n"
    "public func later[] -> Void {}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Allocator::Arena arena;
    EmptyRegistry registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, sources[i], "source-field-failure.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto interpreted =
        dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
    ASSERT(interpreted && interpreted->is<Language::Monograph>());
    auto& monograph = static_cast<Language::Monograph&>(*interpreted);
    auto authored = monograph.get_authored_bindings();
    ASSERT_EQ(authored.get_size(), Count(1));
    ASSERT(authored.get_data()[0].get().is<Language::Function>());
    const auto& later =
        static_cast<const Language::Function&>(authored.get_data()[0].get());

    ASSERT_NOT(monograph.link());
    EXPECT_NOT(later.is_signature_linked());
    EXPECT_NOT(monograph.get_diagnostics().is_empty());
    ASSERT(monograph.get_source().is<Language::Types::Structure>());
    const auto& source_structure =
        static_cast<const Language::Types::Structure&>(monograph.get_source());
    auto fields = source_structure.get_fields();
    if (i == 0) {
      EXPECT(fields.is_empty());
    } else {
      ASSERT_EQ(fields.get_size(), Count(1));
      const Language::Field* identity = &fields.get_data()[0].get();
      ASSERT_NOT(monograph.link());
      EXPECT(&source_structure.get_fields().get_data()[0].get() == identity);
      EXPECT_NOT(later.is_signature_linked());
    }
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, source_categories_share_one_spelling) {
  static constexpr View::Bytes source =
      "public shared : Bool;\n"
      "public func shared[] -> Void { shared; }"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source-field-categories.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  FutureType shared_type("shared"_view);
  ASSERT(monograph.bind_static(shared_type, Language::Visibility::Public));
  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_structure =
      static_cast<const Language::Types::Source&>(monograph.get_source());
  auto fields = source_structure.get_fields();
  auto callables = source_structure.get_callable_bindings();
  ASSERT_EQ(fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(1));
  EXPECT(&monograph.resolve_context("shared"_view) == &shared_type);
  EXPECT(callables.get_data()[0].get().is<Language::Function>());
  const auto& function =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  auto expressions = function.get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(1));
  ASSERT(expressions.get_data()[0].get().is<Language::Identifier>());
  auto selected =
      static_cast<const Language::Identifier&>(expressions.get_data()[0].get())
          .get_addressable();
  ASSERT(selected);
  EXPECT(&*selected == &fields.get_data()[0].get());
  EXPECT(source_structure.get_layout().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, top_level_self_fails_signature_linking) {
  static constexpr View::Bytes source =
      "public func invalid[self] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "top-level-self.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  auto bindings = monograph.get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Function>());
  const auto& function =
      static_cast<const Language::Function&>(bindings.get_data()[0].get());
  EXPECT_NOT(function.is_signature_linked());
  EXPECT(errors.is_empty());

  ASSERT_NOT(monograph.link());
  EXPECT(function.is_signature_linked());
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
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  auto authored = monograph.get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(3));
  const Abstract& hidden = authored.get_data()[0].get();
  const Abstract& public_identity = authored.get_data()[1].get();
  const Abstract& private_identity = authored.get_data()[2].get();
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

  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_structure =
      static_cast<const Language::Types::Source&>(monograph.get_source());
  EXPECT(
      &source_structure.resolve_context("PrivateAlias"_view, monograph) ==
      &private_identity);
  auto bindings = source_structure.get_static_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  EXPECT(&bindings.get_data()[0].get() == &hidden);
  EXPECT(&bindings.get_data()[1].get() == &public_identity);
  EXPECT(&bindings.get_data()[2].get() == &private_identity);

  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  EXPECT(&public_alias.resolve() == &hidden);
  EXPECT(&private_alias.resolve() == &hidden);
  ASSERT(monograph.link());
  ASSERT(monograph.finalize());
  EXPECT(&monograph.resolve_context("PublicAlias"_view) == &public_identity);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(DialectTests, source_alias_routes_and_categories) {
  static constexpr View::Bytes source =
      "public QualifiedAlias : alias = Types::Qualified;\n"
      "public Shared : alias = Bool;"_view;
  Allocator::Arena arena;
  AliasContext registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source-alias-routes.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  auto authored = monograph.get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(2));
  const Abstract& qualified_identity = authored.get_data()[0].get();
  const Abstract& shared_identity = authored.get_data()[1].get();
  ASSERT(qualified_identity.is<Alias>());
  ASSERT(shared_identity.is<Alias>());
  const auto& qualified_alias = static_cast<const Alias&>(qualified_identity);
  EXPECT_TEXT(qualified_alias.get_target().get_name(), "Qualified"_view);
  EXPECT(&qualified_alias.resolve() == &qualified_alias.get_target());
  EXPECT(&shared_identity.resolve() == &Dialect::get_bool());

  AliasAddressable shared_address("Shared"_view, Dialect::get_bool());
  ASSERT(monograph.bind_static(shared_address, Language::Visibility::Public));
  ASSERT(monograph.get_source().is<Language::Types::Structure>());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(monograph.get_source());
  EXPECT(&source_structure.resolve_context("Shared"_view) == &shared_identity);
  auto static_bindings = source_structure.get_static_bindings();
  ASSERT_EQ(static_bindings.get_size(), Count(3));
  EXPECT(&static_bindings.get_data()[1].get() == &shared_identity);
  EXPECT(&static_bindings.get_data()[2].get() == &shared_address);
  EXPECT(errors.is_empty());
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
    auto interpreted =
        dialect.interpret(arena, cursor, Documentation::get_empty(), registry);
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
      "Library Fields require `public`, `private`, or `expose` publication."_view,
      "Library legacy[] -> Void {"_view,
    },
    {
      "validation/data/ttx/library/duplicate_name.ttx"_view,
      "Duplicate Field name in this Library Structure."_view,
      "public duplicate : Unsigned_64 = 2;"_view,
    },
    {
      "validation/data/ttx/library/foreign_named_scope.ttx"_view,
      "Library Type declarations require `alias`, `enum`, `struct`, or `object`."_view,
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
      "public func missing_body[] -> Unsigned_64;"_view,
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
      "Externally readable Structure Callable publishes an unreachable Type "
      "route."_view);
  EXPECT_TEXT(
      diagnostic.get_hint(),
      "Keep the Callable private or publish its authored Type route."_view);
}
