// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/types/namespace.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness PackageDialect = {
  .name = "Tetrodotoxin::Package::Dialect"_view,
};

static auto interpret(
    Allocator::Arena& arena,
    Package::Dialect& dialect,
    Errors& errors,
    View::Bytes source) -> Option<Package::Language::Monograph&> {
  Tokenizer tokenizer(arena, source, "package.ttx"_view);
  Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations, "package.ttx"_view);
  auto selected = dialect.interpret(
      cursor, Documentation::get_empty(), Anchor::create(Span()), dialect);
  return selected ? selected->select<Package::Language::Monograph>()
                  : Option<Package::Language::Monograph&>();
}

static auto has_diagnostic(const Errors& errors, View::Bytes text) -> Bool {
  Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(arena, index), text) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

PERIMORTEM_UNIT_TEST(PackageDialect, type_surface) {
  static constexpr View::Bytes source =
      "public Api : namespace {\n"
      "  public Value : struct { public state number : U64; }\n"
      "}"_view;
  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package(library);
  Errors errors;
  auto monograph = interpret(arena, package, errors, source);
  ASSERT(monograph);

  Tokenizer tokenizer(arena, source, "package.ttx"_view);
  Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations, "package.ttx"_view);
  ASSERT(monograph->compose(cursor));
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));

  const Abstract& api = monograph->resolve_context("Api"_view).resolve();
  ASSERT(api.is<Library::Language::Types::Namespace>());
  const Abstract& value = api.resolve_context("Value"_view).resolve();
  auto structure = value.select<Library::Language::Types::Structure>();
  ASSERT(structure);
  EXPECT_EQ(structure->get_layout().get_size(), Count(1));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, empty_surface) {
  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package(library);
  Errors errors;
  auto monograph = interpret(arena, package, errors, {});
  ASSERT(monograph);

  Tokenizer tokenizer(arena, {}, "package.ttx"_view);
  Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations, "package.ttx"_view);
  EXPECT(monograph->compose(cursor));
  EXPECT(monograph->link(cursor));
  EXPECT(monograph->finalize(cursor));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, rejects_value_members) {
  static constexpr View::Bytes source =
      "public count : U64 = 1;\n"
      "public call : func = [] -> [] : return;"_view;
  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package(library);
  Errors errors;
  auto monograph = interpret(arena, package, errors, source);
  ASSERT(monograph);
  EXPECT(has_diagnostic(
      errors, "Package sources contain only Library Type definitions."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, rejects_manifest_tables) {
  static constexpr View::Bytes source =
      "resolve Math : Perimortem.Math = \"1.0\";\n"
      "source Main from \"main.ttx\";"_view;
  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package(library);
  Errors errors;
  auto monograph = interpret(arena, package, errors, source);
  ASSERT(monograph);
  EXPECT_NOT(errors.is_empty());
}
