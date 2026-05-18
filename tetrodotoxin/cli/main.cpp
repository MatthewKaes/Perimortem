// Perimortem Engine
// Copyright © Matt Kaes

// CLI entry point for the Tetrodotoxin compiler.
//
// Usage:
//   compiler --output <path.a> [--header <path.h>] [<source.ttx> ...]

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "perimortem/utility/null_terminated.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/linker/linker.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;

static auto write_file(const char* path, View::Bytes data) -> Bool {
  System::File file;
  file.update_contents(data);
  return file.write(
      View::Bytes(
          reinterpret_cast<const Byte*>(path), Utility::null_length(path)));
}

int main(int argc, char** argv) {
  const char* output_path = nullptr;
  const char* header_path = nullptr;

  // TODO: Write yet another args parser.
  for (int i = 1; i < argc; i++) {
    if (Utility::null_equal(argv[i], "--output") && i + 1 < argc) {
      output_path = argv[++i];
    } else if (Utility::null_equal(argv[i], "--header") && i + 1 < argc) {
      header_path = argv[++i];
    }
    // TTX source files (remaining args) are recorded for future use.
  }

  if (!output_path) {
    return 1;
  }

  Compiler compiler;

  // TODO: Parse TTX source files from argv and compile their functions.
  // For now, emit a single hardcoded test function that forwards to extern
  // print().
  const Intermediate::Argument view_arg = {
    "data"_view,
    Intermediate::Type::type_view(),
  };
  compiler.compile_function(
      "Hello"_view, "tetrodotoxin_test"_view, Intermediate::Type::type_void(),
      View::Vector<Intermediate::Argument>(&view_arg, 1));

  static constexpr View::Bytes object_name = "ttx.o"_view;
  const Dynamic::Bytes archive = emit_archive(compiler, object_name);
  if (!write_file(output_path, archive.get_view())) {
    return 1;
  }

  if (header_path) {
    Dynamic::Bytes header;
    compiler.generate_cpp_header(header);
    if (!write_file(header_path, header.get_view())) {
      return 1;
    }
  }

  return 0;
}
