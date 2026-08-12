// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/return.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness ReturnTests = {
  .name = "Tetrodotoxin::Library::Language::Return"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  BAIL_IF(!workspace.install_dialect<Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "ReturnTest"_view, "return.ttx"_view, source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return monograph && !workspace.link(errors) && !errors.is_empty();
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

static auto find_return(const Language::Function& function)
    -> Option<const Language::Return&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Reference<Abstract>& statement : body->get_statements()) {
    auto returned = statement.get().select<Language::Return>();
    if (returned) {
      return *returned;
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(ReturnTests, complete_layout_fitting) {
  static constexpr View::Bytes source =
      "// Return Layout flow.\n"
      "dialect : Library;\n"
      "public Empty : struct {}\n"
      "public Packet : struct {\n"
      "  public number : Unsigned_64; public flag : Bool;\n"
      "}\n"
      "public Flow : struct {\n"
      "  public bare_void : func = [] -> Void { return; }\n"
      "  public fallthrough : func = [] -> [] {}\n"
      "  public bare_empty : func = [] -> Empty { return; }\n"
      "  public scalar : func = [] -> Bool { return true; }\n"
      "  public pair : func = [.packet : Packet] -> [Unsigned_64, Bool] {\n"
      "    return packet.[number, flag];\n"
      "  }\n"
      "  public called : func = [.packet : Packet] -> [Unsigned_64, Bool] {\n"
      "    return Flow -> pair(packet);\n"
      "  }\n"
      "  public empty_swizzle : func = [.packet : Packet] -> Empty {\n"
      "    return packet.[];\n"
      "  }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& flow_identity = monograph->resolve_context("Flow"_view);
  ASSERT(flow_identity.is<Language::Types::Structure>());
  const auto& flow =
      static_cast<const Language::Types::Structure&>(flow_identity);
  auto bare_void = find_function(flow, "bare_void"_view);
  auto fallthrough = find_function(flow, "fallthrough"_view);
  auto bare_empty = find_function(flow, "bare_empty"_view);
  auto scalar = find_function(flow, "scalar"_view);
  auto pair = find_function(flow, "pair"_view);
  auto called = find_function(flow, "called"_view);
  auto empty_swizzle = find_function(flow, "empty_swizzle"_view);
  ASSERT(bare_void && fallthrough && bare_empty && scalar && pair && called);
  ASSERT(empty_swizzle);

  auto void_return = find_return(*bare_void);
  auto empty_return = find_return(*bare_empty);
  auto scalar_return = find_return(*scalar);
  auto pair_return = find_return(*pair);
  auto called_return = find_return(*called);
  auto swizzle_return = find_return(*empty_swizzle);
  ASSERT(void_return && !void_return->get_expression());
  ASSERT(empty_return && !empty_return->get_expression());
  ASSERT(scalar_return && scalar_return->get_expression());
  ASSERT(pair_return && pair_return->get_expression());
  ASSERT(called_return && called_return->get_expression());
  ASSERT(swizzle_return && swizzle_return->get_expression());
  EXPECT(
      void_return->get_anchor().get_span().caculate_text(source) ==
      "return;"_view);
  EXPECT(bare_void->get_results().is_empty());
  EXPECT(bare_empty->get_results().is_empty());
  EXPECT(bare_void->get_results().fits(bare_empty->get_results()));
  EXPECT(bare_empty->get_results().fits(bare_void->get_results()));
  ASSERT(fallthrough->get_body());
  EXPECT(fallthrough->get_body()->get_statements().is_empty());
  EXPECT(scalar_return->get_expression()->is<Language::Constants::Flag>());
  EXPECT(pair_return->get_expression()->is<Language::Access::Swizzle>());
  EXPECT(called_return->get_expression()->is<Language::Access::Call>());
  EXPECT(swizzle_return->get_expression()->is<Language::Access::Swizzle>());

  const auto& pair_swizzle = static_cast<const Language::Access::Swizzle&>(
      *pair_return->get_expression());
  const auto& called_call = static_cast<const Language::Access::Call&>(
      *called_return->get_expression());
  const auto& empty_result = static_cast<const Language::Access::Swizzle&>(
      *swizzle_return->get_expression());
  ASSERT_EQ(pair_swizzle.get_results().get_size(), Count(2));
  ASSERT_EQ(called_call.get_results().get_size(), Count(2));
  EXPECT(empty_result.get_results().is_empty());

  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  EXPECT(&*find_return(*scalar) == &*scalar_return);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ReturnTests, incompatible_flow_is_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Missing return.\ndialect : Library; private invalid : func = [] -> Bool {}"_view,
    "// Bare nonempty return.\ndialect : Library; private invalid : func = [] -> Bool { return; }"_view,
    "// Value in empty return.\ndialect : Library; private invalid : func = [] -> [] { return true; }"_view,
    "// Scalar mismatch.\ndialect : Library; private invalid : func = [] -> Bool { return 1; }"_view,
  }};
  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ReturnTests, unreachable_and_incomplete_syntax_roll_back) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Unreachable statement.\ndialect : Library; private invalid : func = [] -> [] { return; Invalid -> call(); }"_view,
    "// Missing return terminator.\ndialect : Library; private invalid : func = [] -> [] { return }"_view,
  }};
  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
