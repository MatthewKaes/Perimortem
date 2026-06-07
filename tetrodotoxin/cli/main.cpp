// Perimortem Engine
// Copyright © Matt Kaes

// CLI entry point for the Tetrodotoxin compiler.
//
// Usage:
//   compiler --output <path.a> [--header <path.h>] [<source.ttx> ...]

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/linker/linker.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;

auto write_file(View::Bytes path, View::Bytes data) -> Bool {
  System::File file;
  file.update_contents(data);
  return file.write(path);
}

Int main(Int argc, SignedBits_8** argv) {
  View::Bytes output_path;
  View::Bytes header_path;

  // TODO: Write yet another args parser.
  for (Count i = 1; i < argc; i++) {
    const auto arg_view = Core::NullTerminated::to_view(argv[i]);
    if (arg_view == "--output"_view && i + 1 < argc) {
      output_path = Core::NullTerminated::to_view(argv[i + 1]);
    } else if (arg_view == "--header"_view && i + 1 < argc) {
      header_path = Core::NullTerminated::to_view(argv[i + 1]);
    }
  }

  if (output_path.empty()) {
    return 1;
  }

  // TODO: Parse TTX source files from argv and compile their functions.
  // For now, emit a single hardcoded test function that forwards to extern
  // print().
  Compiler compiler;
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

  if (!header_path.empty()) {
    Dynamic::Bytes header;
    compiler.generate_cpp_header(header);
    if (!write_file(header_path, header.get_view())) {
      return 1;
    }
  }

  return 0;
}
