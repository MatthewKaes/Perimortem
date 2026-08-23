// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/address.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness AddressTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Address"_view,
};

static auto interpret(
    Environment::Workspace& workspace,
    Errors& errors,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "AddressTest"_view, "address.ttx"_view, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

PERIMORTEM_UNIT_TEST(AddressTests, private_access) {
  static constexpr View::Bytes accepted =
      "// Descendant private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private state secret : Bool;\n"
      "  public Inner : struct {\n"
      "    public read : func = [.outer : Outer] -> Bool {\n"
      "      return outer.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  auto workspace_toolchain = create_library_toolchain();
  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, accepted);
  ASSERT(monograph);
  EXPECT(errors.is_empty());

  static constexpr View::Bytes rejected =
      "// Sibling private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  public Target : struct { private state secret : Bool; }\n"
      "  public Caller : struct {\n"
      "    public read : func = [.target : Outer::Target] -> Bool {\n"
      "      return target.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  auto rejected_toolchain = create_library_toolchain();
  Environment::Workspace rejected_workspace(*rejected_toolchain);
  Errors rejected_errors;
  auto rejected_monograph =
      interpret(rejected_workspace, rejected_errors, rejected);
  EXPECT_NOT(rejected_monograph);
  EXPECT_NOT(rejected_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AddressTests, receiver_storage) {
  static constexpr View::Bytes accepted =
      "// Address receiver categories.\n"
      "dialect : Library;\n"
      "public source_static : Bool = false;\n"
      "public Data : struct {\n"
      "  public static_value : Bool = false;\n"
      "  public state instance_value : Bool;\n"
      "  public const fixed : Bool = false;\n"
      "}\n"
      "private data : Data;\n"
      "private from_source := source.source_static;\n"
      "private from_type := Data.static_value;\n"
      "private from_instance := data.instance_value;\n"
      "private const_from_type := Data.fixed;\n"
      "private const_from_instance := data.fixed;"_view;
  auto workspace_toolchain = create_library_toolchain();
  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, accepted);
  ASSERT(monograph);

  const auto& source = monograph->get_source();
  const auto& data = static_cast<const Library::Language::Types::Composite&>(
      source.resolve_context("Data"_view));
  auto fields = data.get_addressables();
  auto field = fields.begin();
  ASSERT(field != fields.end());
  const Ttx::Concept::Abstract& static_identity = (*field).get();
  ++field;
  ASSERT(field != fields.end());
  const Ttx::Concept::Abstract& state_identity = (*field).get();
  auto layout_state = data.get_layout().get_abstract(0);
  ASSERT(layout_state);
  EXPECT(&*layout_state == &state_identity);
  EXPECT(
      &data.resolve_type_access(
          data, "static_value"_view,
          Library::Language::Model::Type::Access::Static) == &static_identity);
  EXPECT(
      &data.resolve_context("instance_value"_view) ==
      &Ttx::Concept::Invalid::get_invalid());
  EXPECT(errors.is_empty());

  static constexpr Static::Vector<View::Bytes, 2> rejected = {{
    "// Instance rejects Static.\ndialect : Library; public Data : struct { public static_value : Bool = false; } private data : Data; private invalid := data.static_value;"_view,
    "// Type rejects state.\ndialect : Library; public Data : struct { public state instance_value : Bool; } private invalid := Data.instance_value;"_view,
  }};
  for (Count index = 0; index < rejected.get_size(); index++) {
    auto rejected_workspace_toolchain = create_library_toolchain();
    Environment::Workspace rejected_workspace(*rejected_workspace_toolchain);
    Errors rejected_errors;
    auto rejected_monograph =
        interpret(rejected_workspace, rejected_errors, rejected[index]);
    EXPECT_NOT(rejected_monograph);
    EXPECT_NOT(rejected_errors.is_empty());
  }
}
