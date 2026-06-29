// Perimortem Engine
// Copyright © Matt Kaes

// CLI entry point for the Tetrodotoxin compiler.
//
// Usage:
//   compiler --output <path.a> [--header <path.h>] [<source.ttx> ...]

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/shader/spirv.hpp"
#include "tetrodotoxin/compiler/shader/terminal.hpp"
#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

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

auto source_symbol_prefix(View::Bytes source_path) -> Dynamic::Bytes {
  Count start = 0;
  for (Count i = 0; i < source_path.get_size(); i++) {
    if (source_path[i] == '/') {
      start = i + 1;
    }
  }

  Count end = source_path.get_size();
  for (Count i = source_path.get_size(); i > start; i--) {
    if (source_path[i - 1] == '.') {
      end = i - 1;
      break;
    }
  }

  Dynamic::Bytes result;
  result.concat(source_path.slice(start, end - start));
  return result;
}

auto append_object_name(View::Bytes symbol_prefix) -> Dynamic::Bytes {
  Dynamic::Bytes object_name;
  object_name.concat(symbol_prefix);
  object_name.concat("_terminal.o"_view);
  return object_name;
}

auto source_declares_package(View::Bytes source_path) -> Bool {
  System::File source;
  if (!source.read(source_path)) {
    return False;
  }

  return Core::Algorithm::search(source.get_view(), "package"_view) !=
         Count(-1);
}

auto emit_legacy_test_function(Compiler& compiler) -> void {
  const Intermediate::Argument view_arg = {
    "data"_view,
    Intermediate::Type::type_view(),
  };
  compiler.compile_function(
      "Hello"_view, "tetrodotoxin_test"_view, Intermediate::Type::type_void(),
      View::Vector<Intermediate::Argument>(&view_arg, 1));
}

Signed_32 main(Signed_32 argc, Signed_8** argv) {
  View::Bytes output_path;
  View::Bytes header_path;
  Dynamic::Vector<View::Bytes> source_paths;

  // TODO: Write yet another args parser.
  for (Count i = 1; i < argc; i++) {
    const auto arg_view = Core::NullTerminated::to_view(argv[i]);
    if (arg_view == "--output"_view && i + 1 < argc) {
      output_path = Core::NullTerminated::to_view(argv[i + 1]);
      i++;
    } else if (arg_view == "--header"_view && i + 1 < argc) {
      header_path = Core::NullTerminated::to_view(argv[i + 1]);
      i++;
    } else {
      source_paths.insert(arg_view);
    }
  }

  if (output_path.is_empty()) {
    return 1;
  }

  Compiler compiler;
  Dynamic::Bytes header;
  Dynamic::Bytes object_name;
  Bool emitted_shader = False;

  Tetrodotoxin::Resolution::Resolver resolver;
  for (Count i = 0; i < source_paths.get_size(); i++) {
    View::Bytes source_path = source_paths[i];
    if (!resolver.resolve(source_path)) {
      if (source_declares_package(source_path)) {
        return 1;
      }
      continue;
    }

    Tetrodotoxin::Resolution::Record* record = resolver.find(source_path);
    if (!record) {
      return 1;
    }

    if (record->get_package().get_kind() !=
        Tetrodotoxin::Lexical::Class::Type::Shader) {
      continue;
    }

    Dynamic::Bytes vertex_spirv;
    Dynamic::Bytes pixel_spirv;
    if (!Shader::emit_v1_modules(*record, vertex_spirv, pixel_spirv)) {
      return 1;
    }

    Dynamic::Bytes symbol_prefix = source_symbol_prefix(source_path);
    Shader::add_v1_terminal_symbols(
        compiler, *record,
        {
          symbol_prefix,
          vertex_spirv,
          pixel_spirv,
        });

    if (!header_path.is_empty()) {
      Shader::write_v1_terminal_header(*record, symbol_prefix, header);
    }

    if (object_name.is_empty()) {
      object_name = append_object_name(symbol_prefix);
    }
    emitted_shader = True;
  }

  if (!emitted_shader) {
    emit_legacy_test_function(compiler);
    object_name.concat("ttx.o"_view);
    if (!header_path.is_empty()) {
      compiler.generate_cpp_header(header);
    }
  }

  const Dynamic::Bytes archive = emit_archive(
      compiler, object_name.is_empty() ? "ttx.o"_view : object_name.get_view());
  if (!write_file(output_path, archive.get_view())) {
    return 1;
  }

  if (!header_path.is_empty()) {
    if (!write_file(header_path, header.get_view())) {
      return 1;
    }
  }

  return 0;
}
