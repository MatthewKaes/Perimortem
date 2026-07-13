// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Harness TtxArtifact = {
  .name = "Tetrodotoxin::Artifact"_view,
};

PERIMORTEM_UNIT_TEST(TtxArtifact, library) {
  const auto archive = File::read(".bin/bin/validation/ttx_tests.a"_view);
  const auto header =
      File::read(".bin/bin/validation/ttx_generated/ttx_tests.hpp"_view);

  EXPECT_TEXT(archive.get_view().slice(0, 8), "!<arch>\n"_view);
  EXPECT(Algorithm::search(archive, "ttx_tests.o/"_view) != Count(-1));
  EXPECT(
      Algorithm::search(archive, "TTX_source_tetrodotoxin_test"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(archive, "TTX_source_arithmetic"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          header, "extern \"C\" void TTX_source_tetrodotoxin_test"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          header,
          "extern \"C\" Count TTX_source_arithmetic(Count value);"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(header, "TTX_source_sum7"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxArtifact, shader) {
  const auto archive =
      File::read(".bin/bin/validation/shader_artifact_smoke.a"_view);
  static constexpr Static::Bytes<4> spirv_magic = {
    0x03,
    0x02,
    0x23,
    0x07,
  };

  EXPECT_TEXT(archive.get_view().slice(0, 8), "!<arch>\n"_view);
  EXPECT(
      Algorithm::search(
          archive, "TTX_shader_shader_CopyShader_pixel_spirv"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(archive, spirv_magic) != Count(-1));
}
