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
  const auto archive =
      File::read(".bin/bin/validation/Validation.TtxTests/x86_64.a"_view);
  const auto header =
      File::read(".bin/bin/validation/Validation.TtxTests/cpp_abi.hpp"_view);

  EXPECT_TEXT(archive.get_view().slice(0, 8), "!<arch>\n"_view);
  EXPECT(Algorithm::search(archive, "x86_64.o/"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "ttx_internal_"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "ttx_"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "TTX_source_"_view) == Count(-1));

  EXPECT(Algorithm::search(header, "namespace Ttx::Abi"_view) != Count(-1));
  EXPECT(Algorithm::search(header, "struct ViewBytes"_view) != Count(-1));
  EXPECT(Algorithm::search(header, "extern \"C\" void ttx_"_view) != Count(-1));
  EXPECT(Algorithm::search(header, "extern \"C\" Bool"_view) == Count(-1));
  EXPECT(
      Algorithm::search(
          header, "extern \"C\" Perimortem::Core::View::Bytes"_view) ==
      Count(-1));
  EXPECT(
      Algorithm::search(
          header, "inline auto bool_identity(Bool value) -> Bool"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(header, "struct SwapResult"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          header, "namespace Ttx::Validation::TtxTests::Type"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          header, "inline auto arithmetic(Count value) -> Count"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          header,
          "namespace Ttx::Validation::TtxTests::Counter::Addressable"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(header, "TTX_source_"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxArtifact, shader) {
  const auto archive =
      File::read(".bin/bin/validation/Validation.ShaderArtifact/x86_64.a"_view);
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
