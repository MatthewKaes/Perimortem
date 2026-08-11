// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/initializer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "InitializerTest"_view, "initializer.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
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

  return &workspace.resolve_context("InitializerTest"_view) ==
         &Invalid::get_invalid();
}

static Harness InitializerTests = {
  .name = "Tetrodotoxin::Library::Language::Initializer"_view,
};

PERIMORTEM_UNIT_TEST(InitializerTests, empty_and_supplied) {
  static constexpr View::Bytes source =
      "// Initializer test.\n"
      "dialect : Library;\n"
      "public Defaults : object { public enabled : Bool = false; }\n"
      "public Required : object {\n"
      "  public first : Unsigned_64;\n"
      "  private hidden : Bool = false;\n"
      "  expose state second : Bool = false;\n"
      "}\n"
      "public empty : Defaults = new;\n"
      "public configured : Required = new(.second = true, .first = 4);"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(2));
  const auto& defaults =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  const auto& required =
      static_cast<const Language::Types::Object&>(bindings.get_data()[1].get());
  const auto& source_type =
      static_cast<const Language::Types::Source&>(monograph->get_source());
  auto authored_fields = source_type.get_fields();
  ASSERT_EQ(authored_fields.get_size(), Count(2));
  auto empty_initializer =
      authored_fields.get_data()[0].get().get_initializer();
  auto configured_initializer =
      authored_fields.get_data()[1].get().get_initializer();
  ASSERT(empty_initializer);
  ASSERT(configured_initializer);
  ASSERT(empty_initializer->is<Language::Initializer>());
  ASSERT(configured_initializer->is<Language::Initializer>());
  const auto& empty =
      static_cast<const Language::Initializer&>(*empty_initializer);
  const auto& configured =
      static_cast<const Language::Initializer&>(*configured_initializer);
  EXPECT(empty.get_type().is<Invalid>());
  EXPECT(configured.get_type().is<Invalid>());

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&empty.get_type() == &defaults);
  EXPECT(&configured.get_type() == &required);
  EXPECT(empty.get_inputs().is_empty());

  auto fields = required.get_fields();
  ASSERT_EQ(fields.get_size(), Count(3));
  EXPECT_TEXT(fields.get_data()[0].get().get_name(), "first"_view);
  EXPECT_TEXT(fields.get_data()[1].get().get_name(), "hidden"_view);
  EXPECT_TEXT(fields.get_data()[2].get().get_name(), "second"_view);

  const Layout& inputs = configured.get_inputs();
  ASSERT_EQ(inputs.get_size(), Count(2));
  auto second = inputs.get_abstract(0);
  auto first = inputs.get_abstract(1);
  ASSERT(second);
  ASSERT(first);
  ASSERT(second->is<Alias>());
  ASSERT(first->is<Alias>());
  EXPECT_TEXT(second->get_name(), "second"_view);
  EXPECT_TEXT(first->get_name(), "first"_view);
  const auto& second_alias = static_cast<const Alias&>(*second);
  const auto& first_alias = static_cast<const Alias&>(*first);
  EXPECT(second_alias.get_target().is<Language::Expression>());
  EXPECT(first_alias.get_target().is<Language::Expression>());

  Static::Vector<Reference<const Abstract>, 2> initialization_fields = {{
    fields.get_data()[0].get(),
    fields.get_data()[2].get(),
  }};
  Layouts::Named initialization_layout(initialization_fields.get_view());
  EXPECT(inputs.fits(initialization_layout));
  auto fitted_first = inputs.get_fitted(initialization_layout, 0);
  auto fitted_second = inputs.get_fitted(initialization_layout, 1);
  EXPECT(fitted_first.visit(
      [&](const Abstract& selected) {
        return &selected == &*first ? True : False;
      },
      [](Layout::Errors) { return False; }));
  EXPECT(fitted_second.visit(
      [&](const Abstract& selected) {
        return &selected == &*second ? True : False;
      },
      [](Layout::Errors) { return False; }));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, positional_source_order) {
  static constexpr View::Bytes source =
      "// Positional initializer test.\n"
      "dialect : Library;\n"
      "public Required : object {\n"
      "  public first : Unsigned_64;\n"
      "  private hidden : Bool = false;\n"
      "  public second : Bool;\n"
      "}\n"
      "public configured : Required = new(4, true);"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  const auto& source_type =
      static_cast<const Language::Types::Source&>(monograph->get_source());
  auto fields = source_type.get_fields();
  ASSERT_EQ(fields.get_size(), Count(1));
  auto authored_initializer = fields.get_data()[0].get().get_initializer();
  ASSERT(authored_initializer);
  const auto& object_initializer =
      static_cast<const Language::Initializer&>(*authored_initializer);
  const Layout& inputs = object_initializer.get_inputs();
  ASSERT_EQ(inputs.get_size(), Count(2));
  ASSERT(inputs.get_abstract(0));
  ASSERT(inputs.get_abstract(1));
  EXPECT(inputs.get_abstract(0)->is<Language::Expression>());
  EXPECT(inputs.get_abstract(1)->is<Language::Expression>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, inferred_rejected) {
  static constexpr View::Bytes source =
      "// Inferred initializer test.\n"
      "dialect : Library;\n"
      "public Session : object {}\n"
      "public invalid := new;"_view;
  EXPECT(rejects_interpretation(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, wrong_type_rejected) {
  static constexpr View::Bytes source =
      "// Wrong initializer Type test.\n"
      "dialect : Library;\n"
      "public invalid : Bool = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, unknown_name_rejected) {
  static constexpr View::Bytes source =
      "// Unknown initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.missing = 1);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, private_name_rejected) {
  static constexpr View::Bytes source =
      "// Private initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { private hidden : Bool = false; }\n"
      "public invalid : Session = new(.hidden = true);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, duplicate_name_rejected) {
  static constexpr View::Bytes source =
      "// Duplicate initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.value = 1, .value = 2);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, missing_field_rejected) {
  static constexpr View::Bytes source =
      "// Missing initializer input test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, input_type_rejected) {
  static constexpr View::Bytes source =
      "// Initializer input Type test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : Unsigned_64; }\n"
      "public invalid : Session = new(.value = false);"_view;
  EXPECT(rejects_link_without_publication(source));
}

PERIMORTEM_UNIT_TEST(InitializerTests, mandatory_cycle_rejected) {
  static constexpr View::Bytes source =
      "// Initializer cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private next : Node = new; }\n"
      "public invalid : Node = new;"_view;
  EXPECT(rejects_link_without_publication(source));
}
