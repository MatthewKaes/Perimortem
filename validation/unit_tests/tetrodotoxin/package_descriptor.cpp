// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/model/environment.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/package.hpp"
#include "tetrodotoxin/model/packages/interpreted.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "tetrodotoxin/puffer/package/descriptor.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

static Harness PackageDescriptorTests = {
  .name = "Package Descriptor"_view,
};

class DescriptorFixture {
 public:
  constexpr DescriptorFixture(
      View::Bytes path,
      Count resolution_count,
      Count member_count)
      : path(path),
        resolution_count(resolution_count),
        member_count(member_count) {}

  View::Bytes path;
  Count resolution_count;
  Count member_count;
};

class NamedDialect final : public Tetrodotoxin::Model::Dialect {
 public:
  constexpr NamedDialect(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }

  auto evaluate(Ttx::Lexical::Cursor&, const Abstract&) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

static auto parse_descriptor(
    Allocator::Arena& arena,
    View::Bytes path,
    const Abstract& dialects,
    Ttx::Lexical::Errors& errors,
    Dynamic::Bytes& content)
    -> const Tetrodotoxin::Puffer::Package::Descriptor* {
  content = File::read(path);
  if (content.is_empty()) {
    return nullptr;
  }

  Ttx::Lexical::Tokenizer tokenizer(arena, content, path);
  return Tetrodotoxin::Puffer::Package::Descriptor::parse(
      arena, tokenizer, dialects, errors);
}

PERIMORTEM_UNIT_TEST(PackageDescriptorTests, canonical_package_files) {
  constexpr Static::Vector<DescriptorFixture, 5> fixtures = {{
    DescriptorFixture(
        "tetrodotoxin/standard/perimortem/math/package.ttx"_view, 0, 1),
    DescriptorFixture(
        "tetrodotoxin/standard/perimortem/runtime/package.ttx"_view, 0, 1),
    DescriptorFixture(
        "tetrodotoxin/standard/perimortem/graphics/package.ttx"_view, 1, 3),
    DescriptorFixture("apps/ttx/demo/package.ttx"_view, 3, 2),
    DescriptorFixture(
        "validation/data/ttx/shader_artifact/package.ttx"_view, 0, 2),
  }};

  for (Count i = 0; i < fixtures.get_size(); i++) {
    Allocator::Arena arena;
    Tetrodotoxin::Interpreter::Dialects::Package package;
    NamedDialect library("Library"_view);
    NamedDialect render("Render"_view);
    NamedDialect shader("Shader"_view);
    NamedDialect app("App"_view);
    NamedDialect scene("Scene"_view);
    const Static::Vector<Reference<Abstract>, 6> installed = {{
      package,
      library,
      render,
      shader,
      app,
      scene,
    }};
    const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
        arena, "Dialects"_view, installed);
    ASSERT(dialects.is<Tetrodotoxin::Model::Namespace>());
    Dynamic::Bytes content;
    Ttx::Lexical::Errors errors;
    const auto* descriptor =
        parse_descriptor(arena, fixtures[i].path, dialects, errors, content);

    ASSERT(descriptor != nullptr);
    EXPECT(errors.is_empty());
    EXPECT(&descriptor->get_dialect() == &package);
    ASSERT_EQ(descriptor->get_documentation().line_count(), Count(2));
    EXPECT_TEXT(
        descriptor->get_documentation().get_line(0), "Perimortem Engine"_view);
    EXPECT_EQ(
        descriptor->get_resolutions().get_size(), fixtures[i].resolution_count);
    EXPECT_EQ(descriptor->get_members().get_size(), fixtures[i].member_count);
    EXPECT(descriptor->get_body_token_index() > 0);

    for (Count member = 0; member < descriptor->get_members().get_size();
         member++) {
      EXPECT_NOT(descriptor->get_members()[member].get_local_name().is_empty());
      EXPECT_NOT(descriptor->get_members()[member]
                     .get_dialect()
                     .get_name()
                     .is_empty());
      EXPECT_NOT(descriptor->get_members()[member].get_path().is_empty());
    }
    for (Count resolution = 0;
         resolution < descriptor->get_resolutions().get_size(); resolution++) {
      EXPECT(
          descriptor->get_resolutions()[resolution].get_version() ==
          Version(1, 0));
    }
  }
}

PERIMORTEM_UNIT_TEST(PackageDescriptorTests, math_package_evaluates) {
  constexpr View::Bytes path =
      "tetrodotoxin/standard/perimortem/math/package.ttx"_view;
  Allocator::Arena descriptor_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package_dialect;
  NamedDialect library_dialect("Library"_view);
  const Static::Vector<Reference<Abstract>, 2> installed_dialects = {{
    package_dialect,
    library_dialect,
  }};
  const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
      descriptor_arena, "Dialects"_view, installed_dialects);
  ASSERT(dialects.is<Tetrodotoxin::Model::Namespace>());
  Dynamic::Bytes content;
  Ttx::Lexical::Errors errors;
  const auto* descriptor =
      parse_descriptor(descriptor_arena, path, dialects, errors, content);
  ASSERT(descriptor != nullptr);
  ASSERT(errors.is_empty());
  ASSERT_EQ(descriptor->get_members().get_size(), Count(1));
  EXPECT_TEXT(descriptor->get_members()[0].get_local_name(), "Types"_view);
  EXPECT(&descriptor->get_members()[0].get_dialect() == &library_dialect);
  EXPECT_TEXT(
      descriptor->get_members()[0].get_path(), "library/types.ttx"_view);

  Allocator::Arena fixture_arena;
  Types::Unsigned_8 point_type;
  Types::Unsigned_8 size_type;
  Types::Unsigned_8 easing_type;
  Alias point("Point2D"_view, point_type);
  Alias size("Size2D"_view, size_type);
  Alias easing("Easing"_view, easing_type);
  const Static::Vector<Reference<Abstract>, 3> type_exports = {{
    point,
    size,
    easing,
  }};
  const Abstract& types = Tetrodotoxin::Model::Namespace::construct(
      fixture_arena, "Types"_view, type_exports);
  ASSERT(types.is<Tetrodotoxin::Model::Namespace>());

  Tetrodotoxin::Model::Environment environment;
  ASSERT(environment.bind("Types"_view, types, types.get_documentation()));
  Tetrodotoxin::Model::Source source(environment, content, path);

  const Abstract& evaluated = descriptor->evaluate(source, errors);

  ASSERT(errors.is_empty());
  ASSERT(evaluated.is<Tetrodotoxin::Model::Package>());
  ASSERT(evaluated.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT(evaluated.resolve_context("Geometry"_view)
             .is<Tetrodotoxin::Model::Namespace>());
  EXPECT(evaluated.resolve_context("Interpolation"_view)
             .is<Tetrodotoxin::Model::Namespace>());
  EXPECT(
      &evaluated.resolve_context("Geometry"_view)
           .resolve_context("Point2D"_view)
           .resolve() == &point_type);
  EXPECT(
      &evaluated.resolve_context("Geometry"_view)
           .resolve_context("Size2D"_view)
           .resolve() == &size_type);
  EXPECT(
      &evaluated.resolve_context("Interpolation"_view)
           .resolve_context("Easing"_view)
           .resolve() == &easing_type);
}

PERIMORTEM_UNIT_TEST(PackageDescriptorTests, rejects_float_version) {
  Allocator::Arena arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  const Static::Vector<Reference<Abstract>, 1> installed = {{package}};
  const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
      arena, "Dialects"_view, installed);
  Dynamic::Bytes content;
  Ttx::Lexical::Errors errors;

  const auto* descriptor = parse_descriptor(
      arena, "validation/data/ttx/package/float_version.ttx"_view, dialects,
      errors, content);

  EXPECT(descriptor == nullptr);
  ASSERT_EQ(errors.get_size(), Count(1));
  Allocator::Arena render_arena;
  EXPECT(
      Algorithm::search(
          errors.render_message(render_arena, 0),
          "Expected a quoted canonical Major.Minor package version."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(PackageDescriptorTests, rejects_noncanonical_version) {
  Allocator::Arena arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  const Static::Vector<Reference<Abstract>, 1> installed = {{package}};
  const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
      arena, "Dialects"_view, installed);
  Dynamic::Bytes content;
  Ttx::Lexical::Errors errors;

  const auto* descriptor = parse_descriptor(
      arena, "validation/data/ttx/package/noncanonical_version.ttx"_view,
      dialects, errors, content);

  EXPECT(descriptor == nullptr);
  ASSERT_EQ(errors.get_size(), Count(1));
  Allocator::Arena render_arena;
  EXPECT(
      Algorithm::search(
          errors.render_message(render_arena, 0),
          "Expected a canonical non-null package version"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(PackageDescriptorTests, rejects_duplicate_binding) {
  Allocator::Arena arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  const Static::Vector<Reference<Abstract>, 1> installed = {{package}};
  const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
      arena, "Dialects"_view, installed);
  Dynamic::Bytes content;
  Ttx::Lexical::Errors errors;

  const auto* descriptor = parse_descriptor(
      arena, "validation/data/ttx/package/duplicate_binding.ttx"_view, dialects,
      errors, content);

  EXPECT(descriptor == nullptr);
  ASSERT_EQ(errors.get_size(), Count(1));
  Allocator::Arena render_arena;
  EXPECT(
      Algorithm::search(
          errors.render_message(render_arena, 0),
          "Package binding names must be unique"_view) != Count(-1));
}
