// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/value.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
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

static auto import_library(
    Workspace& workspace,
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes source) -> Option<Language::Monograph&> {
  auto imported =
      workspace.import_source(errors, semantic_name, semantic_name, source);
  if (!imported || !imported->is<Language::Monograph>()) {
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
  Dialect dialect(registry);
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

  // Local lookup and public publication borrow the same completed Functions.
  // Reversing declarations changes only each Monograph's authored public order.
  const Abstract& first_alpha = first->resolve_context("alpha"_view);
  const Abstract& first_hidden = first->resolve_context("hidden"_view);
  const Abstract& first_beta = first->resolve_context("beta"_view);
  const Abstract& second_alpha = second->resolve_context("alpha"_view);
  const Abstract& second_hidden = second->resolve_context("hidden"_view);
  const Abstract& second_beta = second->resolve_context("beta"_view);
  ASSERT(
      first_alpha.is<Language::Function>() &&
      first_hidden.is<Language::Function>() &&
      first_beta.is<Language::Function>() &&
      second_alpha.is<Language::Function>() &&
      second_hidden.is<Language::Function>() &&
      second_beta.is<Language::Function>());

  auto first_public = first->get_public_functions();
  auto second_public = second->get_public_functions();
  ASSERT_EQ(first_public.get_size(), Count(2));
  ASSERT_EQ(second_public.get_size(), Count(2));
  EXPECT(&first_public[0].get() == &first_alpha);
  EXPECT(&first_public[1].get() == &first_beta);
  EXPECT(&second_public[0].get() == &second_beta);
  EXPECT(&second_public[1].get() == &second_alpha);
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

  // Both Monographs use the same installed Dialect Types. The reversed source
  // still owns separate Function identities and its own public order.
  for (Count i = 0; i < intrinsic_names.get_size(); i++) {
    const Abstract& first_intrinsic =
        first->resolve_context(intrinsic_names[i]);
    const Abstract& second_intrinsic =
        second->resolve_context(intrinsic_names[i]);
    EXPECT(&first_intrinsic != &Invalid::get_invalid());
    EXPECT(&first_intrinsic == &second_intrinsic);
    EXPECT(first_intrinsic.is<Type>());
    EXPECT_TEXT(first_intrinsic.get_name(), intrinsic_names[i]);
  }

  const Abstract& boolean = first->resolve_context("Bool"_view);
  const Abstract& unsigned_16 = first->resolve_context("Unsigned_16"_view);
  const Abstract& void_type = first->resolve_context("Void"_view);
  ASSERT(void_type.is<Type>());
  EXPECT_NOT(void_type.is<Types::Value>());
  EXPECT(static_cast<const Type&>(void_type).get_layout().is_empty());
  EXPECT_NOT(void_type.get_documentation().is_empty());
  EXPECT(
      &void_type.resolve_context("anything"_view) == &Invalid::get_invalid());

  // Layout edges must reach Dialect identities directly. Named entries add
  // only their authored Alias edge and never copy the intrinsic Type.
  const auto& alpha = static_cast<const Language::Function&>(first_alpha);
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
        return edge.is<Alias>() && edge.get_name() == "value"_view &&
                       &edge.resolve() == &unsigned_16
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

  // Standalone Library sources expose only the approved intrinsic vocabulary.
  // Names from future imports and ambient namespaces remain ordinary misses.
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

  // Workspaces keep independent stateful Dialects while immutable intrinsic
  // Types retain one binary wide identity across every semantic island.
  for (Count i = 0; i < intrinsic_names.get_size(); i++) {
    EXPECT(
        &first->resolve_context(intrinsic_names[i]) ==
        &second->resolve_context(intrinsic_names[i]));
  }
  EXPECT(first_errors.is_empty());
  EXPECT(second_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, duplicate_preserves_first) {
  static constexpr View::Bytes first_source =
      "public func repeated[] -> Void {}"_view;
  static constexpr View::Bytes duplicate_source =
      "public func repeated[Bool] -> Void {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect(registry);
  auto& monograph = arena.construct<Language::Monograph>(
      arena, Documentation::get_empty(), dialect, registry);
  Errors first_errors;
  Tokenizer first_tokenizer(arena, first_source, "first.ttx"_view);
  Cursor first_cursor(first_tokenizer, first_errors);
  auto first = Language::Function::reserve(
      arena, first_cursor, Documentation::get_empty());
  ASSERT(first);

  // Direct binding isolates the mutation contract from Workspace transaction
  // discard. The rejected identity must not disturb either retained view.
  ASSERT(monograph.bind_function(*first));
  ASSERT(first->complete(first_cursor, monograph));
  const Abstract* first_identity = &*first;
  const Count public_size = monograph.get_public_functions().get_size();

  Errors duplicate_errors;
  Tokenizer duplicate_tokenizer(arena, duplicate_source, "duplicate.ttx"_view);
  Cursor duplicate_cursor(duplicate_tokenizer, duplicate_errors);
  auto duplicate = Language::Function::reserve(
      arena, duplicate_cursor, Documentation::get_empty());
  ASSERT(duplicate);
  EXPECT_NOT(monograph.bind_function(*duplicate));
  EXPECT(&monograph.resolve_context("repeated"_view) == first_identity);
  EXPECT_EQ(monograph.get_public_functions().get_size(), public_size);
  EXPECT(&monograph.get_public_functions()[0].get() == first_identity);
  EXPECT_NOT(duplicate->is_complete());

  // Workspace rejection proves that the partially constructed transaction is
  // never published even though its Arena allocation remains safe to discard.
  static constexpr View::Bytes authored_duplicate =
      "// Duplicate Library source.\n"
      "dialect : Library;\n"
      "public func repeated[] -> Void {}\n"
      "public func repeated[Bool] -> Void {}"_view;
  Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  EXPECT_NOT(workspace.import_source(
      errors, "Duplicate"_view, "duplicate-source.ttx"_view,
      authored_duplicate));
  EXPECT(
      &workspace.resolve_context("Duplicate"_view) == &Invalid::get_invalid());
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, rejected_sources_publish_nothing) {
  static constexpr Static::Vector<View::Bytes, 4> rejected = {{
    "// Malformed prefix.\n"
    "dialect : Library;\n"
    "public broken[] -> Void {}"_view,
    "// Incomplete signature.\n"
    "dialect : Library;\n"
    "public func broken[Bool] ->"_view,
    "// Bodyless definition.\n"
    "dialect : Library;\n"
    "public func broken[] -> Void;"_view,
    "// Trailing invalid syntax.\n"
    "dialect : Library;\n"
    "public func valid[] -> Void {} trailing;"_view,
  }};
  Workspace workspace;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));

  // A failed source never enters Workspace lookup, so the same exact semantic
  // name can exercise each independent rejection without replacement state.
  for (Count i = 0; i < rejected.get_size(); i++) {
    Errors errors;
    EXPECT_NOT(workspace.import_source(
        errors, "Rejected"_view, "rejected.ttx"_view, rejected[i]));
    EXPECT(
        &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());
    EXPECT_NOT(errors.is_empty());
  }
}
