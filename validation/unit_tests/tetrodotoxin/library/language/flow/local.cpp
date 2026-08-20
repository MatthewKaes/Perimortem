// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/local.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LocalTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::Local"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
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
  auto workspace_toolchain = create_library_toolchain();
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty();
}

static auto rejects_link_without_publication(View::Bytes source) -> Bool {
  auto workspace_toolchain = create_library_toolchain();
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty()) {
    return False;
  }

  return &workspace.resolve_context("LocalTest"_view) ==
         &Invalid::get_invalid();
}

PERIMORTEM_UNIT_TEST(LocalTests, source_order_and_type_completion) {
  static constexpr View::Bytes source =
      "// Local outcomes.\n"
      "dialect : Library;\n"
      "public Packet : object { public state enabled : Bool = false; }\n"
      "public Pair : struct { public state left : Bool; public state right : "
      "Bool; }\n"
      "public body : func = [] -> Bool {\n"
      "  state explicit : Bool = true;\n"
      "  const fixed : Bool = false;\n"
      "  const inferred := fixed;\n"
      "  state copied := fixed;\n"
      "  state created := new[Packet];\n"
      "  state positional : Pair = (true, false);\n"
      "  state named : Pair = (.right = false, .left = true);\n"
      "  return inferred;\n"
      "}"_view;
  auto workspace_toolchain = create_library_toolchain();
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  const Abstract& packet_identity = source_type.resolve_context("Packet"_view);
  const Abstract& pair_identity = source_type.resolve_context("Pair"_view);
  ASSERT(packet_identity.is<Language::Types::Object>());
  ASSERT(pair_identity.is<Language::Types::Structure>());
  auto body = find_function(source_type, "body"_view);
  ASSERT(body && body->get_body());

  const Language::Flow::Block& block = *body->get_body();
  auto statements = block.get_statements();
  ASSERT_EQ(statements.get_size(), Count(8));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[2].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[3].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[4].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[5].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[6].get_root().is<Language::Flow::Local>());
  const auto& explicit_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  const auto& fixed_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[1].get_root());
  const auto& inferred_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[2].get_root());
  const auto& copied_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[3].get_root());
  const auto& created_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[4].get_root());
  const auto& positional_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[5].get_root());
  const auto& named_local = static_cast<const Language::Flow::Local&>(
      statements.get_data()[6].get_root());
  const Abstract& boolean = monograph->resolve_context("Bool"_view);

  EXPECT(&explicit_local.get_type() == &boolean);
  EXPECT(&fixed_local.get_type() == &boolean);
  EXPECT(&inferred_local.get_type() == &boolean);
  EXPECT(&copied_local.get_type() == &boolean);
  EXPECT(&created_local.get_type() == &packet_identity);
  EXPECT(&positional_local.get_type() == &pair_identity);
  EXPECT(&named_local.get_type() == &pair_identity);
  EXPECT(explicit_local.get_writability() == Language::Writability::Full);
  EXPECT(fixed_local.get_writability() == Language::Writability::Constant);
  EXPECT(inferred_local.get_writability() == Language::Writability::Constant);
  EXPECT(copied_local.get_writability() == Language::Writability::Full);
  EXPECT(created_local.get_writability() == Language::Writability::Full);
  EXPECT(positional_local.get_writability() == Language::Writability::Full);
  EXPECT(named_local.get_writability() == Language::Writability::Full);
  ASSERT(fixed_local.get_constant());
  ASSERT(inferred_local.get_constant());
  EXPECT(&*fixed_local.get_constant() == &*inferred_local.get_constant());

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

  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "local.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  auto repeated = block.get_statements();
  ASSERT_EQ(repeated.get_size(), statements.get_size());
  for (Count i = 0; i < statements.get_size(); i++) {
    EXPECT(
        &repeated.get_data()[i].get_root() ==
        &statements.get_data()[i].get_root());
  }
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LocalTests, const_fixed_retains_folded_values) {
  static constexpr View::Bytes source =
      "// Const Fixed Local.\n"
      "dialect : Library;\n"
      "public body : func = [] -> Unsigned_64 {\n"
      "  const dense : Fixed[Unsigned_64, 4] = (5, 6, 7, 8);\n"
      "  const extracted := dense:[1];\n"
      "  return extracted;\n"
      "}"_view;
  auto workspace_toolchain = create_library_toolchain();
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto body = find_function(monograph->get_source(), "body"_view);
  ASSERT(body && body->get_body());
  auto statements = body->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(3));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::Local>());
  const auto& dense = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  const auto& extracted = static_cast<const Language::Flow::Local&>(
      statements.get_data()[1].get_root());
  EXPECT(dense.get_writability() == Language::Writability::Constant);
  EXPECT(extracted.get_writability() == Language::Writability::Constant);

  auto folded = dense.get_constant();
  ASSERT(folded);
  ASSERT_EQ(folded->get_layout().get_size(), Count(4));
  for (Count index = 0; index < Count(4); index++) {
    auto produced = folded->get_produced(index);
    ASSERT(produced);
    auto value = produced->producer.select<Language::Constants::Unsigned>();
    ASSERT(value);
    EXPECT_EQ(value->get_value(), Unsigned_64(index + 5));
  }
  auto extracted_value = extracted.get_constant();
  ASSERT(extracted_value);
  auto value = extracted_value->select<Language::Constants::Unsigned>();
  ASSERT(value);
  EXPECT_EQ(value->get_value(), Unsigned_64(6));
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
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Duplicate Local.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool; const value := true; }"_view,
    "// Inferred construction.\ndialect : Library; private invalid : func = [] -> [] { state value := new; }"_view,
    "// Missing const value.\ndialect : Library; private invalid : func = [] -> [] { const value : Bool; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(LocalTests, invalid_type_flow_is_not_published) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Forward Local.\ndialect : Library; private invalid : func = [] -> Bool { const first := later; const later : Bool = true; return first; }"_view,
    "// Mismatched Local.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool = 1; }"_view,
    "// Empty Local.\ndialect : Library; public Empty : struct {} private invalid : func = [] -> [] { state value : Empty; }"_view,
    "// Incomplete structural Pack.\ndialect : Library; public Pair : struct { public left : Bool; public right : Bool; } private invalid : func = [] -> [] { state value : Pair = (.left = true); }"_view,
    "// Dynamic const Local.\ndialect : Library; private invalid : func = [] -> Bool { state mutable : Bool = true; const value := mutable; return value; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link_without_publication(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(
    LocalTests,
    explicit_type_survives_initializer_diagnostics) {
  static constexpr View::Bytes source =
      "// Typed Local diagnostic recovery.\n"
      "dialect : Library;\n"
      "public Pair : struct {\n"
      "  public state left : Unsigned_64 = 2;\n"
      "  public state right : Unsigned_64 = 3;\n"
      "  public sum : func = [self] -> Unsigned_64 {\n"
      "    return self.left + self.right;\n"
      "  }\n"
      "}\n"
      "public execute : func = [] -> Unsigned_64 {\n"
      "  state pair : Pair = (.left = 2, .right2 = 3);\n"
      "  state total : Unsigned_64 = pair -> sum();\n"
      "  return total;\n"
      "}"_view;
  auto workspace_toolchain = create_library_toolchain();
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  EXPECT_NOT(interpret(workspace, errors, source));
  EXPECT(
      &workspace.resolve_context("LocalTest"_view) == &Invalid::get_invalid());
  ASSERT_EQ(errors.get_size(), Count(1));

  Perimortem::Memory::Allocator::Arena rendered;
  View::Bytes diagnostic = errors.render_message(rendered, 0);
  EXPECT(Algorithm::search(diagnostic, "right2"_view) != Count(-1));
  EXPECT(Algorithm::search(diagnostic, "Pair"_view) != Count(-1));
  EXPECT(Algorithm::search(diagnostic, "Identifier 'pair'"_view) == Count(-1));
  EXPECT(Algorithm::search(diagnostic, "Identifier 'total'"_view) == Count(-1));
}
