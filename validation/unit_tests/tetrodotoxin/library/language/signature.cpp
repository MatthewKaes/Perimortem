// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/signature.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness SignatureTests = {
  .name = "Tetrodotoxin::Library::Language::Signature"_view,
};

static auto interpret(
    Workspace& workspace,
    Ttx::Lexical::Errors& errors,
    View::Bytes source) -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "SignatureTest"_view, "signature.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_function(
    Language::Types::Composite& composite,
    View::Bytes name) -> Option<Language::Function&> {
  for (Reference<Abstract> candidate : composite.get_callables()) {
    if (candidate.get().get_name() == name &&
        candidate.get().is<Language::Function>()) {
      return static_cast<Language::Function&>(candidate.get());
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(SignatureTests, named_parameters_and_direct_results) {
  static constexpr View::Bytes source =
      "// Signature identity test.\n"
      "dialect : Library;\n"
      "public inspect : func = [.input : Bool,] -> [\n"
      "    .count : Unsigned_64, .accepted : Bool,\n"
      "  ] { return (.count = 0, .accepted = false); }"_view;
  Workspace workspace;
  Ttx::Lexical::Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "inspect"_view);
  ASSERT(function);

  const Layout& parameters = function->get_parameters();
  const Layout& results = function->get_results();

  ASSERT_EQ(parameters.get_size(), Count(1));
  ASSERT(parameters.get_name(0));
  EXPECT_TEXT(*parameters.get_name(0), "input"_view);
  auto parameter = parameters.get_abstract(0);
  ASSERT(parameter && parameter->is<Language::Parameter>());
  const auto& input = static_cast<const Language::Parameter&>(*parameter);
  EXPECT(&input.get_type() == &monograph->resolve_context("Bool"_view));

  ASSERT_EQ(results.get_size(), Count(2));
  ASSERT(results.get_name(0) && results.get_name(1));
  EXPECT_TEXT(*results.get_name(0), "count"_view);
  EXPECT_TEXT(*results.get_name(1), "accepted"_view);
  EXPECT(
      &*results.get_abstract(0) ==
      &monograph->resolve_context("Unsigned_64"_view));
  EXPECT(
      &*results.get_abstract(1) ==
      &monograph->resolve_context("Bool"_view));
  EXPECT(results.get_abstract(0)->is<Ttx::Model::Type>());
  EXPECT(results.get_abstract(1)->is<Ttx::Model::Type>());

  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SignatureTests, descriptor_shape_is_strict) {
  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Bare parameter.\ndialect : Library; private invalid : func = Bool -> [] {}"_view,
    "// Positional parameter.\ndialect : Library; private invalid : func = [Bool] -> [] {}"_view,
    "// Mixed parameter.\ndialect : Library; public Packet : struct { public value : Bool; private invalid : func = [self, Bool] -> [] {} }"_view,
    "// Duplicate parameter.\ndialect : Library; private invalid : func = [.value : Bool, .value : Bool] -> [] {}"_view,
    "// Value-label separator.\ndialect : Library; private invalid : func = [.value = Bool] -> [] {}"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Workspace workspace;
    Ttx::Lexical::Errors errors;
    EXPECT_NOT(interpret(workspace, errors, rejected[i]));
    EXPECT_NOT(errors.is_empty());
    EXPECT(
        &workspace.resolve_context("SignatureTest"_view) ==
        &Invalid::get_invalid());
  }
}
