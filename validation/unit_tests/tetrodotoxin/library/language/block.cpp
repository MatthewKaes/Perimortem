// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/block.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness BlockTests = {
  .name = "Tetrodotoxin::Library::Language::Block"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "BlockTest"_view, "block.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (auto binding = composite.get_callables().begin();
       binding != composite.get_callables().end(); ++binding) {
    const Abstract& candidate = (*binding).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(BlockTests, authored_scope_and_order) {
  static constexpr View::Bytes source =
      "// Block owner.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public touch : func = [] -> Void { return; }\n"
      "  public empty : func = [] -> Void {}\n"
      "  public body : func = [.input : Unsigned_64] -> Unsigned_64 {\n"
      "    (Packet -> touch());\n"
      "    return input;\n"
      "  }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& packet_identity =
      monograph->get_source().resolve_context("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  auto empty = find_function(packet, "empty"_view);
  auto body = find_function(packet, "body"_view);
  ASSERT(empty && empty->get_body());
  ASSERT(body && body->get_body());

  const Language::Block& empty_block = *empty->get_body();
  const Language::Block& populated = *body->get_body();
  EXPECT(empty_block.get_statements().is_empty());
  EXPECT(
      empty_block.get_anchor().get_span().caculate_text(source) == "{}"_view);
  EXPECT(
      populated.get_anchor().get_span().caculate_text(source) ==
      "{\n    (Packet -> touch());\n    return input;\n  }"_view);

  auto statements = populated.get_statements();
  ASSERT_EQ(statements.get_size(), Count(2));
  const Abstract& first = statements.get_data()[0].get();
  const Abstract& second = statements.get_data()[1].get();
  EXPECT(&first != &second);
  ASSERT(first.is<Language::Access::Call>());
  ASSERT(second.is<Language::Return>());
  const auto& invoked = static_cast<const Language::Access::Call&>(first);
  const auto& returned = static_cast<const Language::Return&>(second);
  EXPECT(invoked.get_callable());
  EXPECT_NOT(invoked.get_folded());
  EXPECT_TEXT(
      returned.get_anchor().get_span().caculate_text(source),
      "return input;"_view);

  auto parameter = body->get_parameters().get_abstract(0);
  ASSERT(parameter);
  EXPECT(&populated.resolve_context("input"_view) == &*parameter);

  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  auto repeated = populated.get_statements();
  ASSERT_EQ(repeated.get_size(), Count(2));
  EXPECT(&repeated.get_data()[0].get() == &first);
  EXPECT(&repeated.get_data()[1].get() == &second);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BlockTests, non_invocations_are_not_statements) {
  static constexpr Static::Vector<View::Bytes, 6> sources = {{
    "// Constant statement.\ndialect : Library; private invalid : func = [] -> [] { true; }"_view,
    "// Operation statement.\ndialect : Library; private invalid : func = [] -> [] { 1 + 2; }"_view,
    "// Address statement.\ndialect : Library; public Packet : struct { public value : Bool; } private packet : Packet; private invalid : func = [] -> [] { packet.value; }"_view,
    "// Type statement.\ndialect : Library; private invalid : func = [] -> [] { Bool; }"_view,
    "// Composed invocation Pack.\ndialect : Library; private Packet : struct { private touch : func = [] -> [] {} } private invalid : func = [] -> [] { (Packet -> touch(), Packet -> touch()); }"_view,
    "// Named invocation Pack.\ndialect : Library; private Packet : struct { private touch : func = [] -> [] {} } private invalid : func = [] -> [] { (.result = Packet -> touch()); }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    EXPECT_NOT(interpret(workspace, errors, sources[i]));
    EXPECT_NOT(errors.is_empty());
    EXPECT(
        &workspace.resolve_context("BlockTest"_view) ==
        &Invalid::get_invalid());
  }
}

PERIMORTEM_UNIT_TEST(BlockTests, failed_scope_is_not_published) {
  static constexpr View::Bytes source =
      "// Block rollback.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public body : func = [.input : Unsigned_64] -> Unsigned_64 {\n"
      "    return input;\n"
      "    input;\n"
      "  }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  EXPECT_NOT(interpret(workspace, errors, source));
  EXPECT_NOT(errors.is_empty());
  EXPECT(
      &workspace.resolve_context("BlockTest"_view) == &Invalid::get_invalid());
}
