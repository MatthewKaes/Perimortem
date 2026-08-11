// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness TypeAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Type"_view,
};

static auto rejects_type_access(View::Bytes source) -> Bool {
  Allocator::Arena domain;
  Errors errors;
  Tokenizer tokenizer(domain, source, "type-access-rejection.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed = Library::Language::Access::Type::parse(cursor);
  return !parsed && !errors.is_empty();
}

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
      "  private Hidden : struct {}\n"
      "  public Inner : struct { private value : Outer::Hidden; }\n"
      "}"_view;
  static constexpr View::Bytes rejected =
      "// External private Type access.\n"
      "dialect : Library;\n"
      "public Outer : struct { private Hidden : struct {} }\n"
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
      "  public Bool : struct { public Nested : struct {} }\n"
      "  public value : Bool::Nested;\n"
      "}"_view;

  EXPECT(links_library_source(source));
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, grammar) {
  EXPECT(rejects_type_access("Root ::Nested"_view));
  EXPECT(rejects_type_access("Root:: Nested"_view));
  EXPECT(rejects_type_access("Root::"_view));
  EXPECT(rejects_type_access("Root::value"_view));
  EXPECT(rejects_type_access("value"_view));
}
