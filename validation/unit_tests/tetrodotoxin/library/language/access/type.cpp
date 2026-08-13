// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness TypeAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Type"_view,
};

static auto links_library_source(View::Bytes source) -> Bool {
  Environment::Workspace workspace;
  Errors errors;
  BAIL_IF(!workspace.install_dialect<Library::Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "TypeAccessTest"_view, "type-access.ttx"_view, source);
  return interpreted && workspace.link(errors) && workspace.finalize(errors) &&
         errors.is_empty();
}

static auto rejects_library_link(View::Bytes source) -> Bool {
  Environment::Workspace workspace;
  Errors errors;
  BAIL_IF(!workspace.install_dialect<Library::Dialect>("Library"_view));
  auto interpreted = workspace.interpret_source(
      errors, "TypeAccessTest"_view, "type-access.ttx"_view, source);
  return interpreted && !workspace.link(errors) && !errors.is_empty();
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, qualified_private_authority) {
  static constexpr View::Bytes accepted =
      "// Qualified private Type access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private Hidden : struct { private value : Bool; }\n"
      "  public Inner : struct { private value : Outer::Hidden; }\n"
      "}"_view;
  static constexpr View::Bytes rejected =
      "// External private Type access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private Hidden : struct { private value : Bool; }\n"
      "}\n"
      "public Borrowed : alias = Outer;\n"
      "private invalid : Borrowed::Hidden;"_view;

  EXPECT(links_library_source(accepted));
  EXPECT(rejects_library_link(rejected));
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, local_root_shadows_intrinsic) {
  static constexpr View::Bytes source =
      "// Local Type root shadowing.\n"
      "dialect : Library;\n"
      "public Host : struct {\n"
      "  public Bool : struct {\n"
      "    public Nested : struct { private value : Unsigned_8; }\n"
      "  }\n"
      "  public value : Bool::Nested;\n"
      "}"_view;

  EXPECT(links_library_source(source));
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, expression_result_drives_access) {
  static constexpr View::Bytes accepted =
      "// Type-valued expression access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  public Nested : struct {\n"
      "    public create : func = [] -> Bool { return true; }\n"
      "  }\n"
      "}\n"
      "private selected := Outer::Nested -> create();"_view;
  static constexpr View::Bytes value_qualification =
      "// Value cannot provide Type qualification.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private value : Bool;\n"
      "  public Nested : struct {}\n"
      "}\n"
      "private value : Outer;\n"
      "private invalid := value::Nested;"_view;
  static constexpr View::Bytes descriptor_address =
      "// A Structure Type descriptor does not prove static storage.\n"
      "dialect : Library;\n"
      "public Packet : struct { expose state value : Bool = false; }\n"
      "private invalid := Packet.value;"_view;
  static constexpr View::Bytes object_descriptor_address =
      "// An Object Type descriptor does not prove static storage.\n"
      "dialect : Library;\n"
      "public Packet : object { public value : Bool = false; }\n"
      "private invalid := Packet.value;"_view;
  static constexpr View::Bytes const_descriptor_address =
      "// A Structure Type selects its compile-time const Field.\n"
      "dialect : Library;\n"
      "public Packet : struct { public const value : Bool = false; }\n"
      "private selected := Packet.value;"_view;
  static constexpr View::Bytes const_object_descriptor_address =
      "// An Object Type selects its compile-time const Field.\n"
      "dialect : Library;\n"
      "public Packet : object { public const value : Bool = false; }\n"
      "private selected := Packet.value;"_view;
  static constexpr View::Bytes source_address =
      "// Source Fields have unambiguous static storage.\n"
      "dialect : Library;\n"
      "public value : Bool = false;\n"
      "private selected := source.value;"_view;

  EXPECT(links_library_source(accepted));
  EXPECT(rejects_library_link(value_qualification));
  EXPECT(rejects_library_link(descriptor_address));
  EXPECT(rejects_library_link(object_descriptor_address));
  EXPECT(links_library_source(const_descriptor_address));
  EXPECT(links_library_source(const_object_descriptor_address));
  EXPECT(links_library_source(source_address));
}
