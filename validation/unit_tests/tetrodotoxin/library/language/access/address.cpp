// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
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
  if (!workspace.install_dialect<Library::Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "AddressTest"_view, "address.ttx"_view, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

PERIMORTEM_UNIT_TEST(AddressTests, descendant_private_authority) {
  static constexpr View::Bytes source =
      "// Descendant private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private secret : Bool;\n"
      "  public Inner : struct {\n"
      "    public read : func = [.outer : Outer] -> Bool {\n"
      "      return outer.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AddressTests, sibling_private_denied) {
  static constexpr View::Bytes source =
      "// Sibling private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  public Target : struct { private secret : Bool; }\n"
      "  public Caller : struct {\n"
      "    public read : func = [.target : Outer::Target] -> Bool {\n"
      "      return target.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  EXPECT_NOT(workspace.link(errors));
  EXPECT_NOT(errors.is_empty());
}
