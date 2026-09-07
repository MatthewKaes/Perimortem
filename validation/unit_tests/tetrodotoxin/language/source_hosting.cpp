// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include <string>

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/reference.hpp"
#include "tetrodotoxin/terminal/graph_text.hpp"
#include "ttx/model/layouts/value.hpp"
#include "validation/cross_language/source.h"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness Sources = {.name = "Tetrodotoxin::Sources"_view};

class NativeFrontend : public Language::Dialect {
 public:
  explicit NativeFrontend(View::Bytes name) : Dialect(name) {}
  size_t interpretations = 0;
  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor&,
      ttx_abstract context) -> Option<Language::Monograph&> override {
    ++interpretations;
    if (!cursor.matches(Ttx::Lexical::Code::Type::Terminal)) {
      cursor.create_token_error(
          "This fixture accepts common imports only."_view);
    }
    return cursor.get_arena().construct<Language::Monograph>(
        cursor.get_arena(), get_abi(), documentation, context);
  }
};

static auto string_value(ttx_abstract value) -> std::string {
  const auto bytes = Ttx::copy_bytes(value);
  return bytes ? std::string(bytes->begin(), bytes->end()) : std::string();
}

PERIMORTEM_UNIT_TEST(Sources, foreign_generation_replacement) {
  EXPECT(validation_source_live_graphs() == 0);
  Environment::Toolchain toolchain;
  auto installed = toolchain.install<NativeFrontend>("Native"_view);
  ASSERT(installed);
  const auto foreign = validation_source_provider();
  EXPECT(toolchain.install(foreign));
  {
    Environment::Workspace workspace(toolchain);
    const auto state = workspace.interpret_source(
        toolchain.find("Native"_view), "native"_view, "native.ttx"_view,
        "// A source that imports an unrelated frontend.\ndialect : Native;\n"
        "// The external graph.\npublic Foreign : alias = source(\"foreign\");\n"_view);
    EXPECT(state == Language::InterpretationState::Constructed);
    auto native = workspace.observe_source("native"_view);
    EXPECT(
        Language::validate(native.get()).state ==
        Language::ValidationState::Incomplete);

    EXPECT(
        workspace.interpret_source(
            foreign, "foreign"_view, "foreign.data"_view, "first"_view) ==
        Language::InterpretationState::Constructed);
    EXPECT(
        Language::validate(native.get()).state ==
        Language::ValidationState::Accepted);
    EXPECT(installed->interpretations == 1);
    EXPECT(workspace.source_revision("foreign"_view) == 1);

    Language::Reference imported(native.root(), "Foreign"_view);
    Language::Reference value(imported.get_abi(), "value"_view);
    const auto first = Ttx::resolve(value.get_abi());
    EXPECT(string_value(first) == "first");
    auto pins = workspace.retain_sources();
    const auto context = ttx_context_create();
    Ttx::Layouts::Value produced(first);
    const auto old = Ttx::pack(context, produced.get_abi());
    ASSERT(old.state == Ttx::PackObservationState::Packed);

    EXPECT(
        workspace.interpret_source(
            foreign, "foreign"_view, "foreign.data"_view, "second"_view) ==
        Language::InterpretationState::Constructed);
    EXPECT(workspace.source_revision("foreign"_view) == 2);
    const auto second = Ttx::resolve(value.get_abi());
    EXPECT(!ttx_abstract_same(first, second));
    EXPECT(string_value(second) == "second");
    const auto old_values = Ttx::producers(old.pack);
    ASSERT(old_values && old_values->size() == 1);
    EXPECT(ttx_abstract_same((*old_values)[0], first));
    EXPECT(string_value((*old_values)[0]) == "first");
    EXPECT(validation_source_live_graphs() == 2);
    EXPECT(installed->interpretations == 1);

    // The old Pack borrows the first C allocation. Release that borrow before
    // dropping its source pins, then check that Workspace keeps no edit
    // history.
    context.operations->release(context);
    pins.clear();
    EXPECT(validation_source_live_graphs() == 1);

    const auto report = Terminal::GraphText::write(
        {}, installed->get_abi(), native.root(), workspace.get_abi());
    const std::string graph(report.begin(), report.end());
    EXPECT(graph.find("466f726569676e") != std::string::npos);

    EXPECT(
        workspace.interpret_source(
            toolchain.find("Native"_view), "native"_view, "native.ttx"_view,
            "// An incomplete source.\ndialect : Native;\nunexpected"_view) ==
        Language::InterpretationState::Constructed);
    auto partial = workspace.observe_source("native"_view);
    EXPECT(
        Language::validate(partial.get()).state ==
        Language::ValidationState::Incomplete);
    size_t reports = 0;
    static const tetrodotoxin_source_diagnostics_ops diagnostics = {
      .header =
          {sizeof(tetrodotoxin_source_diagnostics_ops), TTX_ABI_MAJOR,
           TTX_ABI_MINOR},
      .diagnostic =
          [](tetrodotoxin_source_diagnostics sink, tetrodotoxin_source_anchor,
             ttx_borrowed_bytes) { ++*reinterpret_cast<size_t*>(sink.self); },
      .completed = [](tetrodotoxin_source_diagnostics) {},
    };
    partial.get().operations->visit_diagnostics(
        partial.get().self,
        {&diagnostics,
         reinterpret_cast<tetrodotoxin_source_diagnostics_self*>(&reports)});
    EXPECT(reports != 0);
    EXPECT(string_value(second) == "second");
  }
  EXPECT(validation_source_live_graphs() == 0);
}

PERIMORTEM_UNIT_TEST(Sources, source_diagnostics_without_a_root) {
  Environment::Toolchain toolchain;
  auto installed = toolchain.install<NativeFrontend>("Native"_view);
  ASSERT(installed);
  Environment::Workspace workspace(toolchain);
  const auto provider = toolchain.find("Native"_view);
  EXPECT(
      workspace.interpret_source(
          provider, "source"_view, "source.ttx"_view,
          "dialect : Native;"_view) ==
      Language::InterpretationState::Constructed);
  auto incomplete = workspace.observe_source("source"_view);
  EXPECT(ttx_abstract_same(incomplete.root(), ttx_unknown()));
  EXPECT(
      Language::validate(incomplete.get()).state ==
      Language::ValidationState::Incomplete);
  EXPECT(installed->interpretations == 0);

  size_t reports = 0;
  static const tetrodotoxin_source_diagnostics_ops diagnostics = {
    .header =
        {sizeof(tetrodotoxin_source_diagnostics_ops), TTX_ABI_MAJOR,
         TTX_ABI_MINOR},
    .diagnostic =
        [](tetrodotoxin_source_diagnostics sink, tetrodotoxin_source_anchor,
           ttx_borrowed_bytes message) {
          *reinterpret_cast<size_t*>(sink.self) += message.size != 0;
        },
    .completed = [](tetrodotoxin_source_diagnostics) {},
  };
  incomplete.get().operations->visit_diagnostics(
      incomplete.get().self,
      {&diagnostics,
       reinterpret_cast<tetrodotoxin_source_diagnostics_self*>(&reports)});
  EXPECT(reports == 1);

  EXPECT(
      workspace.interpret_source(
          provider, "source"_view, "source.ttx"_view,
          "//\ndialect : Native;"_view) ==
      Language::InterpretationState::Constructed);
  auto complete = workspace.observe_source("source"_view);
  EXPECT(
      Language::validate(complete.get()).state ==
      Language::ValidationState::Accepted);
  EXPECT(installed->interpretations == 1);
  EXPECT(ttx_abstract_same(incomplete.root(), ttx_unknown()));
}
