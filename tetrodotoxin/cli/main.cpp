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
  Count file_name_start = 0;
  for (Count i = 0; i < source_path.get_size(); i++) {
    if (source_path[i] == '/') {
      file_name_start = i + 1;
    }
  }

  Count extension_start = source_path.get_size();
  for (Count i = source_path.get_size(); i > file_name_start; i--) {
    if (source_path[i - 1] == '.') {
      extension_start = i - 1;
      break;
    }
  }

  Dynamic::Bytes symbol_prefix;
  symbol_prefix.concat(
      source_path.slice(file_name_start, extension_start - file_name_start));
  return symbol_prefix;
}

auto append_object_name(View::Bytes symbol_prefix) -> Dynamic::Bytes {
  Dynamic::Bytes object_name;
  object_name.concat(symbol_prefix);
  object_name.concat("_terminal.o"_view);
  return object_name;
}

auto append_library_object_name(View::Bytes symbol_prefix) -> Dynamic::Bytes {
  Dynamic::Bytes object_name;
  object_name.concat(symbol_prefix);
  object_name.concat("_library.o"_view);
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

auto last_type_name(const Tetrodotoxin::Syntax::Type::Ref& type)
    -> View::Bytes {
  View::Vector<View::Bytes> path = type.get_name_path();
  return path.is_empty() ? View::Bytes() : path[path.get_size() - 1];
}

auto is_view_bytes(const Tetrodotoxin::Syntax::Type::Ref& type) -> Bool {
  if (last_type_name(type) != "View"_view) {
    return False;
  }

  View::Vector<Tetrodotoxin::Syntax::Type::Ref> type_parameters =
      type.get_params();
  return type_parameters.get_size() == 1 &&
         last_type_name(type_parameters[0]) == "Bytes"_view;
}

auto abi_type_for(
    const Tetrodotoxin::Syntax::Type::Ref& type,
    Abi::Type& abi_type_out) -> Bool {
  if (is_view_bytes(type)) {
    abi_type_out = Abi::Type::view_bytes();
    return True;
  }

  return False;
}

auto abi_return_for(
    const Tetrodotoxin::Syntax::Ast::Layout& return_layout,
    Abi::Type& abi_return_type_out) -> Bool {
  if (return_layout.is_value() && return_layout.get_value_type().is_empty()) {
    abi_return_type_out = Abi::Type::void_type();
    return True;
  }

  if (return_layout.is_value() &&
      last_type_name(return_layout.get_value_type()) == "Void"_view) {
    abi_return_type_out = Abi::Type::void_type();
    return True;
  }

  return abi_type_for(return_layout.get_value_type(), abi_return_type_out);
}

auto abi_arguments_for(
    const Tetrodotoxin::Syntax::Ast::Layout& parameter_layout,
    Dynamic::Vector<Abi::Argument>& abi_arguments_out) -> Bool {
  if (parameter_layout.is_value() &&
      parameter_layout.get_value_type().is_empty()) {
    return True;
  }

  if (!parameter_layout.is_named()) {
    return False;
  }

  View::Vector<Tetrodotoxin::Syntax::Ast::Param> layout_parameters =
      parameter_layout.get_params();
  for (Count i = 0; i < layout_parameters.get_size(); i++) {
    Abi::Type abi_type;
    if (!abi_type_for(layout_parameters[i].get_type(), abi_type)) {
      return False;
    }

    abi_arguments_out.insert({
      layout_parameters[i].get_name(),
      abi_type,
    });
  }

  return True;
}

auto emit_library_package(
    Compiler& compiler,
    const Tetrodotoxin::Resolution::Record& record,
    View::Bytes module_name) -> Bool {
  // Keep this bridge honest: until expression lowering exists, Library output
  // only accepts ABI shapes the current x86-64 stub can actually call.
  Bool emitted = False;
  View::Vector<Tetrodotoxin::Syntax::Ast::Function*> package_functions =
      record.get_package().get_functions();

  for (Count i = 0; i < package_functions.get_size(); i++) {
    const Tetrodotoxin::Syntax::Ast::Function* package_function =
        package_functions[i];
    if (!package_function || package_function->is_disabled() ||
        package_function->is_external()) {
      continue;
    }

    Dynamic::Vector<Abi::Argument> abi_arguments;
    Abi::Type abi_return_type;
    if (!abi_arguments_for(package_function->get_params(), abi_arguments) ||
        !abi_return_for(package_function->get_returns(), abi_return_type)) {
      return False;
    }

    if (!compiler.compile_function(
            module_name, package_function->get_definition().get_name(),
            abi_return_type, abi_arguments.get_view(),
            package_function->get_body())) {
      return False;
    }
    emitted = True;
  }

  return emitted;
}

Signed_32 main(Signed_32 argc, Signed_8** argv) {
  View::Bytes output_path;
  View::Bytes header_path;
  Dynamic::Vector<View::Bytes> source_paths;

  // TODO: Write yet another args parser.
  for (Count i = 1; i < argc; i++) {
    const auto argument_view = Core::NullTerminated::to_view(argv[i]);
    if (argument_view == "--output"_view && i + 1 < argc) {
      output_path = Core::NullTerminated::to_view(argv[i + 1]);
      i++;
    } else if (argument_view == "--header"_view && i + 1 < argc) {
      header_path = Core::NullTerminated::to_view(argv[i + 1]);
      i++;
    } else {
      source_paths.insert(argument_view);
    }
  }

  if (output_path.is_empty()) {
    return 1;
  }

  Compiler compiler;
  Dynamic::Bytes header;
  Dynamic::Bytes object_name;
  Bool emitted_shader = False;
  Bool emitted_library = False;

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

    Dynamic::Bytes symbol_prefix = source_symbol_prefix(source_path);
    const Tetrodotoxin::Lexical::Class package_kind =
        record->get_package().get_kind();

    if (package_kind == Tetrodotoxin::Lexical::Class::Type::Library) {
      if (!emit_library_package(compiler, *record, symbol_prefix)) {
        return 1;
      }

      if (object_name.is_empty()) {
        object_name = append_library_object_name(symbol_prefix);
      }
      emitted_library = True;
      continue;
    }

    if (package_kind != Tetrodotoxin::Lexical::Class::Type::Shader) {
      continue;
    }

    Dynamic::Bytes vertex_spirv;
    Dynamic::Bytes pixel_spirv;
    if (!Shader::emit_v1_modules(*record, vertex_spirv, pixel_spirv)) {
      return 1;
    }

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

  if (!emitted_shader && !emitted_library) {
    return 1;
  }

  if (emitted_library) {
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
