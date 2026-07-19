// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/compiler.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/puffer/resolution/context.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer;
using namespace Validation;

static Harness PufferCompiler = {
  .name = "Puffer::Compiler"_view,
};

PERIMORTEM_UNIT_TEST(PufferCompiler, library_output) {
  static constexpr Static::Vector<View::Bytes, 1> sources = {{
    "validation/unit_tests/tetrodotoxin/ttx/source.ttx"_view,
  }};
  Resolution::Context context;
  Compilation output = Compiler::build_library(
      context, "Validation.Direct"_view, View::Vector<View::Bytes>(), sources);

  EXPECT_NOT(context.has_errors());
  EXPECT_NOT(output.get_native_archive().is_empty());
  EXPECT_NOT(output.get_cpp_header().is_empty());
  EXPECT(output.get_package_buffer().is_empty());
}

PERIMORTEM_UNIT_TEST(PufferCompiler, package_output) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "tetrodotoxin/standard/perimortem/math/package.ttx"_view,
    "tetrodotoxin/standard/perimortem/math/library/types.ttx"_view,
  }};
  static constexpr Static::Vector<View::Bytes, 2> reordered_sources = {{
    "tetrodotoxin/standard/perimortem/math/library/types.ttx"_view,
    "tetrodotoxin/standard/perimortem/math/package.ttx"_view,
  }};
  Resolution::Context context;
  Compilation output = Compiler::build_package(
      context, "Perimortem.Math"_view, View::Vector<View::Bytes>(), sources);
  Resolution::Context reordered_context;
  Compilation reordered = Compiler::build_package(
      reordered_context, "Perimortem.Math"_view, View::Vector<View::Bytes>(),
      reordered_sources);

  EXPECT_NOT(context.has_errors());
  EXPECT_NOT(reordered_context.has_errors());
  EXPECT_NOT(output.get_native_archive().is_empty());
  EXPECT_NOT(output.get_cpp_header().is_empty());
  EXPECT_NOT(output.get_package_buffer().is_empty());
  EXPECT(output.get_native_archive() == reordered.get_native_archive());
  EXPECT(output.get_cpp_header() == reordered.get_cpp_header());
  EXPECT(output.get_package_buffer() == reordered.get_package_buffer());
}

PERIMORTEM_UNIT_TEST(PufferCompiler, missing_package_root) {
  static constexpr Static::Vector<View::Bytes, 1> sources = {{
    "tetrodotoxin/standard/perimortem/math/library/types.ttx"_view,
  }};
  Resolution::Context context;
  Compilation output = Compiler::build_package(
      context, "Perimortem.Math"_view, View::Vector<View::Bytes>(), sources);

  EXPECT(context.has_errors());
  EXPECT(output.get_native_archive().is_empty());
  EXPECT(output.get_cpp_header().is_empty());
  EXPECT(output.get_package_buffer().is_empty());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Package compilation needs a package.ttx root."_view);
}
