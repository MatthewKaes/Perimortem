// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/option.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness OptionAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Option"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "OptionAccessTest"_view, "option-access.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  for (const Reference<Abstract>& candidate : composite.get_addressables()) {
    if (candidate.get().get_name() == name &&
        candidate.get().is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate.get());
    }
  }

  return {};
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return monograph && !workspace.link(errors) && !errors.is_empty() &&
         &workspace.resolve_context("OptionAccessTest"_view) ==
             &Invalid::get_invalid();
}

static auto select_unsigned(const Language::Model::Pack& pack)
    -> Option<const Language::Constants::Unsigned&> {
  auto direct = pack.select<Language::Constants::Unsigned>();
  if (direct) {
    return *direct;
  }

  auto entry = pack.get_layout().get_abstract(0);
  return entry.visit(
      []() -> Option<const Language::Constants::Unsigned&> { return {}; },
      [](const Abstract& selected) {
        return selected.select<Language::Constants::Unsigned>();
      });
}

PERIMORTEM_UNIT_TEST(OptionAccessTests, target_fit_and_unwrap) {
  static constexpr View::Bytes source =
      "// Option target fitting and unwrap.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[Unsigned_64];\n"
      "private const absent : Maybe = ();\n"
      "private const present : Maybe = 7;\n"
      "private const absent_copy : Maybe = absent;\n"
      "private const present_copy : Maybe = present;\n"
      "private const fallback := absent!;\n"
      "private const selected := present!;\n"
      "public Options : struct {\n"
      "  public consume : func = [.value : Maybe] -> Maybe { return value; }\n"
      "}\n"
      "private absent_call := Options -> consume(());\n"
      "private present_call := Options -> consume(8);"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& source_type = monograph->get_source();
  auto absent = find_field(source_type, "absent"_view);
  auto present = find_field(source_type, "present"_view);
  auto fallback = find_field(source_type, "fallback"_view);
  auto selected = find_field(source_type, "selected"_view);
  auto absent_copy = find_field(source_type, "absent_copy"_view);
  auto present_copy = find_field(source_type, "present_copy"_view);
  auto absent_call = find_field(source_type, "absent_call"_view);
  auto present_call = find_field(source_type, "present_call"_view);
  ASSERT(absent && present && absent_copy && present_copy);
  ASSERT(fallback && selected);
  ASSERT(absent_call && present_call);

  auto absent_constant = absent->get_constant();
  auto present_constant = present->get_constant();
  auto absent_copy_constant = absent_copy->get_constant();
  auto present_copy_constant = present_copy->get_constant();
  ASSERT(absent_constant && present_constant);
  ASSERT(absent_copy_constant && present_copy_constant);
  EXPECT(&*absent_copy_constant == &*absent_constant);
  EXPECT(&*present_copy_constant == &*present_constant);
  auto absent_option = absent_constant->select<Language::Constants::Option>();
  auto present_option = present_constant->select<Language::Constants::Option>();
  ASSERT(absent_option && present_option);
  EXPECT(absent_option->get_kind() == Language::Types::Option::Kind::Absent);
  EXPECT_NOT(absent_option->get_payload());
  EXPECT(present_option->get_kind() == Language::Types::Option::Kind::Present);
  auto payload = present_option->get_payload();
  ASSERT(payload);
  auto payload_value = select_unsigned(*payload);
  ASSERT(payload_value);
  EXPECT_EQ(payload_value->get_value(), Unsigned_64(7));

  auto fallback_constant = fallback->get_constant();
  auto selected_constant = selected->get_constant();
  ASSERT(fallback_constant && selected_constant);
  auto fallback_value = select_unsigned(*fallback_constant);
  auto selected_value = select_unsigned(*selected_constant);
  ASSERT(fallback_value && selected_value);
  EXPECT_EQ(fallback_value->get_value(), Unsigned_64(0));
  EXPECT_EQ(selected_value->get_value(), Unsigned_64(7));

  EXPECT(absent_call->get_type().is<Language::Types::Option>());
  EXPECT(&absent_call->get_type() == &present_call->get_type());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(OptionAccessTests, propagation_accepts_empty_result_flow) {
  static constexpr View::Bytes source =
      "// Option propagation.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[Unsigned_64];\n"
      "public pass : func = [.value : Maybe] -> Maybe { return value?; }\n"
      "public stop : func = [.value : Maybe] -> [] {\n"
      "  state selected := value?;\n"
      "  return;\n"
      "}\n"
      "public absent : func = [] -> Maybe { return; }\n"
      "public present : func = [] -> Maybe { return 9; }\n"
      "public assign : func = [] -> Maybe {\n"
      "  state value : Maybe = ();\n"
      "  value = 4;\n"
      "  value = ();\n"
      "  return value;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(OptionAccessTests, invalid_elimination_is_rejected) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Propagation result mismatch.\ndialect : Library; private invalid : func = [.value : Option[Unsigned_64]] -> Bool { return value?; }"_view,
    "// Propagation receiver mismatch.\ndialect : Library; private invalid : func = [.value : Bool] -> [] { state selected := value?; return; }"_view,
    "// Unwrap receiver mismatch.\ndialect : Library; private invalid : func = [.value : Bool] -> Bool { return value!; }"_view,
    "// Option slice.\ndialect : Library; private invalid : func = [.value : Option[Unsigned_64]] -> Unsigned_64 { return value:[0]; }"_view,
    "// Some constructor.\ndialect : Library; private Maybe : alias = Option[Unsigned_64]; private invalid := Maybe -> some(1);"_view,
    "// Empty constructor.\ndialect : Library; private Maybe : alias = Option[Unsigned_64]; private invalid := Maybe -> empty();"_view,
    "// Option target mismatch.\ndialect : Library; private invalid : Option[Unsigned_64] = false;"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index]));
  }
}
