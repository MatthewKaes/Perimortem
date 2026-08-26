// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness StandardMathPackage = {
  .name = "Tetrodotoxin::Package::Math"_view,
};

static auto decode(Allocator::Arena& arena, View::Bytes bytes)
    -> Option<Package::Archive::Archive> {
  return Package::Archive::Reader::read(arena, bytes)
      .visit(
          [](const Package::Archive::Archive& selected)
              -> Option<Package::Archive::Archive> { return selected; },
          [](const Package::Archive::Reader::Error&)
              -> Option<Package::Archive::Archive> { return {}; });
}

static auto select_vector(
    const Package::Language::Monograph& package,
    View::Bytes name) -> Option<const Library::Language::Types::Structure&> {
  return package.resolve_context(name)
      .resolve()
      .select<Library::Language::Types::Structure>();
}

PERIMORTEM_UNIT_TEST(StandardMathPackage, vector_layouts) {
  auto product =
      File::read(".bin/bin/packages/ttx/Perimortem.Math/1.0/contract.txa"_view);
  ASSERT(product);
  Allocator::Arena archive_arena;
  auto archive = decode(archive_arena, *product);
  ASSERT(archive);

  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  ASSERT(library);
  ASSERT(toolchain.install<Package::Dialect>("Package"_view, *library));
  Environment::Workspace workspace(toolchain);
  auto restored = workspace.restore_package(*archive, "Math"_view);
  ASSERT(restored && restored->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*restored);

  auto vec2 = select_vector(package, "Vec2D"_view);
  auto vec3 = select_vector(package, "Vec3D"_view);
  auto vec4 = select_vector(package, "Vec4D"_view);
  ASSERT(vec2 && vec3 && vec4);
  EXPECT_TEXT(
      vec2->get_documentation().get_line(0),
      "Vec2D stores two ordered R32 components named x and y."_view);
  EXPECT_TEXT(
      vec3->get_documentation().get_line(0),
      "Vec3D stores three ordered R32 components named x, y, and z."_view);
  EXPECT_TEXT(
      vec4->get_documentation().get_line(0),
      "Vec4D stores four ordered R32 components named x, y, z, and w."_view);
  EXPECT_EQ(vec2->get_layout().get_size(), Count(2));
  EXPECT_EQ(vec3->get_layout().get_size(), Count(3));
  EXPECT_EQ(vec4->get_layout().get_size(), Count(4));
  static constexpr Static::Vector<View::Bytes, 4> names = {{
    "x"_view,
    "y"_view,
    "z"_view,
    "w"_view,
  }};
  Count field_count = 0;
  for (const auto& field :
       vec4->get_addressables(Tetrodotoxin::Language::Visibility::Public)) {
    ASSERT(field_count < names.get_size());
    EXPECT_TEXT(field.get().get_name(), names[field_count]);
    field_count++;
  }
  EXPECT_EQ(field_count, names.get_size());
}
