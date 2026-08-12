// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/local.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LocalTests = {
  .name = "Tetrodotoxin::Library::Language::Local"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "LocalTest"_view, "local.ttx"_view, source);
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

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty();
}

static auto rejects_link_without_publication(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || workspace.link(errors) || errors.is_empty()) {
    return False;
  }

  return &workspace.resolve_context("LocalTest"_view) ==
         &Invalid::get_invalid();
}

PERIMORTEM_UNIT_TEST(LocalTests, source_order_and_type_completion) {
  static constexpr View::Bytes source =
      "// Local outcomes.\n"
      "dialect : Library;\n"
      "public Packet : object { public enabled : Bool = false; }\n"
      "public Pair : struct { public left : Bool; public right : Bool; }\n"
      "public body : func = [] -> Bool {\n"
      "  state explicit : Bool = true;\n"
      "  const fixed : Bool = false;\n"
      "  const inferred := explicit;\n"
      "  state copied := fixed;\n"
      "  state created : Packet = new;\n"
      "  state positional : Pair = (true, false);\n"
      "  state named : Pair = (.right = false, .left = true);\n"
      "  return inferred;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& source_type = monograph->get_source();
  const Abstract& packet_identity = source_type.resolve_context("Packet"_view);
  const Abstract& pair_identity = source_type.resolve_context("Pair"_view);
  ASSERT(packet_identity.is<Language::Types::Object>());
  ASSERT(pair_identity.is<Language::Types::Structure>());
  auto body = find_function(source_type, "body"_view);
  ASSERT(body && body->get_body());

  const Language::Block& block = *body->get_body();
  auto statements = block.get_statements();
  ASSERT_EQ(statements.get_size(), Count(8));
  ASSERT(statements.get_data()[0].get().is<Language::Local>());
  ASSERT(statements.get_data()[1].get().is<Language::Local>());
  ASSERT(statements.get_data()[2].get().is<Language::Local>());
  ASSERT(statements.get_data()[3].get().is<Language::Local>());
  ASSERT(statements.get_data()[4].get().is<Language::Local>());
  ASSERT(statements.get_data()[5].get().is<Language::Local>());
  ASSERT(statements.get_data()[6].get().is<Language::Local>());
  const auto& explicit_local =
      static_cast<const Language::Local&>(statements.get_data()[0].get());
  const auto& fixed_local =
      static_cast<const Language::Local&>(statements.get_data()[1].get());
  const auto& inferred_local =
      static_cast<const Language::Local&>(statements.get_data()[2].get());
  const auto& copied_local =
      static_cast<const Language::Local&>(statements.get_data()[3].get());
  const auto& created_local =
      static_cast<const Language::Local&>(statements.get_data()[4].get());
  const auto& positional_local =
      static_cast<const Language::Local&>(statements.get_data()[5].get());
  const auto& named_local =
      static_cast<const Language::Local&>(statements.get_data()[6].get());

  EXPECT(&explicit_local.get_type() == &Dialect::get_bool());
  EXPECT(&fixed_local.get_type() == &Dialect::get_bool());
  EXPECT(&inferred_local.get_type() == &Dialect::get_bool());
  EXPECT(&copied_local.get_type() == &Dialect::get_bool());
  EXPECT(&created_local.get_type() == &packet_identity);
  EXPECT(&positional_local.get_type() == &pair_identity);
  EXPECT(&named_local.get_type() == &pair_identity);
  EXPECT(explicit_local.get_writability() == Language::Writability::Full);
  EXPECT(fixed_local.get_writability() == Language::Writability::Init);
  EXPECT(inferred_local.get_writability() == Language::Writability::Init);
  EXPECT(copied_local.get_writability() == Language::Writability::Full);
  EXPECT(created_local.get_writability() == Language::Writability::Full);
  EXPECT(positional_local.get_writability() == Language::Writability::Full);
  EXPECT(named_local.get_writability() == Language::Writability::Full);
  EXPECT(
      explicit_local.get_anchor().get_span().caculate_text(source) ==
      "state explicit : Bool = true;"_view);
  EXPECT(&block.resolve_context("explicit"_view) == &explicit_local);
  EXPECT(&block.resolve_context("fixed"_view) == &fixed_local);
  EXPECT(&block.resolve_context("inferred"_view) == &inferred_local);
  EXPECT(&block.resolve_context("copied"_view) == &copied_local);
  EXPECT(&block.resolve_context("created"_view) == &created_local);
  EXPECT(&block.resolve_context("positional"_view) == &positional_local);
  EXPECT(&block.resolve_context("named"_view) == &named_local);

  ASSERT(monograph->link());
  auto repeated = block.get_statements();
  ASSERT_EQ(repeated.get_size(), statements.get_size());
  for (Count i = 0; i < statements.get_size(); i++) {
    EXPECT(&repeated.get_data()[i].get() == &statements.get_data()[i].get());
  }
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LocalTests, inference_requires_one_scalar_value) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Empty inferred Pack.\ndialect : Library; private invalid : func = [] -> [] { const value := (); }"_view,
    "// Multi-value inferred Pack.\ndialect : Library; private invalid : func = [] -> [] { const value := (true, false); }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link_without_publication(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(LocalTests, malformed_declarations_are_atomic) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Duplicate Local.\ndialect : Library; private invalid : func = [] -> Void { state value : Bool; const value := true; }"_view,
    "// Inferred construction.\ndialect : Library; private invalid : func = [] -> Void { state value := new; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(LocalTests, invalid_type_flow_is_not_published) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Forward Local.\ndialect : Library; private invalid : func = [] -> Bool { const first := later; const later : Bool = true; return first; }"_view,
    "// Mismatched Local.\ndialect : Library; private invalid : func = [] -> Void { state value : Bool = 1; }"_view,
    "// Empty Local.\ndialect : Library; private invalid : func = [] -> Void { state value : Void; }"_view,
    "// Incomplete structural Pack.\ndialect : Library; public Pair : struct { public left : Bool; public right : Bool; } private invalid : func = [] -> Void { state value : Pair = (.left = true); }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link_without_publication(sources[i]));
  }
}
