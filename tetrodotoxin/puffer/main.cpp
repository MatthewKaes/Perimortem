// Perimortem Engine
// Copyright © Matt Kaes

#include <stdio.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/args.hpp"
#include "perimortem/system/file.hpp"

#include "tetrodotoxin/compiler/library.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"
#include "tetrodotoxin/resolution/source/record.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Resolution;

class Puffer {
 public:
  Puffer() : library_compiler(arena) {}

  auto run(View::Vector<View::Bytes> command_line) -> Signed_32 {
    Bool help_requested = requested_help(command_line);
    Allocator::Arena args_arena;
    Args::Values options = Args::parse(
        args_arena, help_summary, create_args(args_arena), command_line);
    if (options.is_empty()) {
      return help_requested ? 0 : 2;
    }

    if (!configure(options)) {
      return 2;
    }

    Toolchain toolchain = Toolchain::standard();
    Resolver resolver(toolchain);
    Resolver::Context context;

    if (!load_dependencies(options, resolver, context) ||
        !load_sources(options, resolver, context)) {
      print_errors(context);
      return 1;
    }

    if (terminal_count == 0) {
      fprintf(stderr, "puffer: no TTX roots were selected for output\n");
      return 1;
    }

    Tetrodotoxin::Linker::Linker linker;
    library_compiler.add_to(linker);

    View::Bytes output_path = arg_value(options, "-output"_view);
    Dynamic::Bytes archive = linker.build_library("ttx_terminal.o"_view);
    if (!write_file(output_path, archive.get_view())) {
      fprintf(stderr, "puffer: failed to write archive ");
      print_bytes(stderr, output_path);
      fprintf(stderr, "\n");
      return 1;
    }

    View::Bytes header_path = arg_value(options, "-header"_view);
    Dynamic::Bytes header = build_header();
    if (!write_file(header_path, header.get_view())) {
      fprintf(stderr, "puffer: failed to write header ");
      print_bytes(stderr, header_path);
      fprintf(stderr, "\n");
      return 1;
    }

    View::Bytes puffer_path = arg_value(options, "-puffer"_view);
    if (!write_file(puffer_path, terminal_data.get_view())) {
      fprintf(stderr, "puffer: failed to write puffer sidecar ");
      print_bytes(stderr, puffer_path);
      fprintf(stderr, "\n");
      return 1;
    }

    return 0;
  }

 private:
  using Configs = Managed::Map<View::Bytes, Args::Config>;

  static constexpr View::Bytes help_summary =
      "Compile TTX sources into archives and puffer sidecars for Bazel."_view;

  static auto print_bytes(FILE* file, View::Bytes bytes) -> void {
    fprintf(
        file, "%.*s", Signed_32(bytes.get_size()),
        Data::cast<const char>(bytes.get_data()));
  }

  static auto requested_help(View::Vector<View::Bytes> command_line) -> Bool {
    for (Count i = 1; i < command_line.get_size(); i++) {
      if (command_line[i] == "--help"_view || command_line[i] == "-help"_view ||
          command_line[i] == "-h"_view) {
        return True;
      }
    }
    return False;
  }

  static auto create_args(Allocator::Arena& arena) -> Configs {
    Configs variables(arena);
    variables.insert(
        "-library"_view,
        {.help = "Compile source roots as TTX library terminals."_view});
    variables.insert(
        "-package"_view,
        {.help = "Compile package manifests as TTX package terminals."_view});
    variables.insert(
        "-output"_view,
        {
            .help = "Write the output static archive."_view,
            .required = True,
        });
    variables.insert(
        "-header"_view,
        {
            .help = "Write the generated C++ header."_view,
            .required = True,
        });
    variables.insert(
        "-puffer"_view,
        {
            .help = "Write the puffer terminal sidecar."_view,
            .required = True,
        });
    variables.insert(
        "-dep"_view,
        {.help = "Make a dependency source visible while loading."_view});
    variables.insert(
        "-source"_view,
        {
            .help = "TTX source roots to compile."_view,
            .required = True,
        });
    return variables;
  }

  static auto arg_values(const Args::Values& args, View::Bytes name)
      -> View::Vector<View::Bytes> {
    const auto* entry = args.find(name);
    if (entry == nullptr) {
      return View::Vector<View::Bytes>();
    }

    return entry->value->get_view();
  }

  static auto arg_value(const Args::Values& args, View::Bytes name)
      -> View::Bytes {
    View::Vector<View::Bytes> values = arg_values(args, name);
    return values.is_empty() ? View::Bytes() : values[0];
  }

  auto configure(const Args::Values& args) -> Bool {
    Bool library = args.contains("-library"_view);
    package = args.contains("-package"_view);
    if (library == package) {
      Diagnostics::Log::error(
          "puffer: select exactly one of -library or -package\n"_view,
          Diagnostics::Source());
      return False;
    }

    return True;
  }

  static auto is_package_root(View::Bytes source_path) -> Bool {
    constexpr View::Bytes package_file = "package.ttx"_view;
    if (source_path == package_file) {
      return True;
    }

    return source_path.get_size() > package_file.get_size() &&
           source_path[source_path.get_size() - package_file.get_size() - 1] ==
               '/' &&
           source_path.slice(
               source_path.get_size() - package_file.get_size(),
               package_file.get_size()) == package_file;
  }

  auto load_dependencies(
      const Args::Values& args,
      Resolver& resolver,
      Resolver::Context& context) -> Bool {
    View::Vector<View::Bytes> dependencies = arg_values(args, "-dep"_view);
    for (Count i = 0; i < dependencies.get_size(); i++) {
      View::Bytes dependency = dependencies[i];
      if (package && !is_package_root(dependency)) {
        continue;
      }

      if (resolver.load_source(context, dependency) == nullptr) {
        return False;
      }
    }

    return True;
  }

  auto load_sources(
      const Args::Values& args,
      Resolver& resolver,
      Resolver::Context& context) -> Bool {
    Count selected_sources = 0;
    View::Vector<View::Bytes> sources = arg_values(args, "-source"_view);
    for (Count i = 0; i < sources.get_size(); i++) {
      View::Bytes source = sources[i];
      if (package && !is_package_root(source)) {
        continue;
      }

      selected_sources++;
      Source::Record* record = resolver.load_source(context, source);
      if (record == nullptr) {
        return False;
      }

      add_terminal(*record);
      if (!package && record->get_type() != nullptr &&
          !library_compiler.lower(
              build_module_name(record->get_source_path()),
              *record->get_type())) {
        fprintf(stderr, "puffer: ");
        print_bytes(stderr, library_compiler.get_error());
        fprintf(stderr, "\n");
        return False;
      }
    }

    if (package && selected_sources == 0) {
      fprintf(stderr, "puffer: package mode needs a package.ttx source\n");
      return False;
    }

    return True;
  }

  auto add_terminal(Source::Record& record) -> void {
    append_field("source"_view, record.get_source_path());
    append_field("import"_view, record.get_import_name());
    append_field("isa"_view, record.get_boot().get_isa());
    if (record.get_type() != nullptr) {
      append_type_facts(*record.get_type(), record.get_type()->get_name());
    }

    terminal_count++;
  }

  auto append_field(View::Bytes name, View::Bytes value) -> void {
    terminal_data.concat(name);
    terminal_data.append(':');
    terminal_data.append(' ');
    terminal_data.concat(value);
    terminal_data.append('\n');
  }

  auto append_type_facts(const Ttx::Type& type, View::Bytes path) -> void {
    append_field("type"_view, path);

    View::Vector<Ttx::Type::Member> members = type.get_members();
    for (Count i = 0; i < members.get_size(); i++) {
      append_member_fact(path, members[i]);
    }

    View::Vector<Ttx::Type::Function> functions = type.get_functions();
    for (Count i = 0; i < functions.get_size(); i++) {
      append_function_fact(path, functions[i]);
    }

    View::Vector<const Ttx::Type*> types = type.get_types();
    for (Count i = 0; i < types.get_size(); i++) {
      if (types[i] == nullptr) {
        continue;
      }

      Dynamic::Bytes nested_path;
      nested_path.concat(path);
      nested_path.concat("::"_view);
      nested_path.concat(types[i]->get_name());
      append_type_facts(*types[i], nested_path.get_view());
    }
  }

  auto append_member_fact(
      View::Bytes owner,
      const Ttx::Type::Member& member) -> void {
    terminal_data.concat("member: "_view);
    terminal_data.concat(owner);
    terminal_data.concat("."_view);
    terminal_data.concat(member.get_name().is_empty() ? "_"_view
                                                      : member.get_name());
    terminal_data.concat(" "_view);
    terminal_data.concat(type_name(member.get_type()));
    terminal_data.append('\n');
  }

  auto append_function_fact(
      View::Bytes owner,
      const Ttx::Type::Function& function) -> void {
    terminal_data.concat("function: "_view);
    terminal_data.concat(owner);
    terminal_data.concat("->"_view);
    terminal_data.concat(function.get_name());
    terminal_data.concat(" params="_view);
    append_decimal(terminal_data, function.get_parameters().get_size());
    terminal_data.concat(" result="_view);
    append_decimal(terminal_data, function.get_result().get_size());
    terminal_data.concat(" blocks="_view);
    append_decimal(terminal_data, function.get_blocks().get_size());
    terminal_data.append('\n');
  }

  static auto type_name(const Ttx::Type* type) -> View::Bytes {
    return type == nullptr ? "<unresolved>"_view : type->get_name();
  }

  auto build_module_name(View::Bytes source_path) -> View::Bytes {
    Count start = 0;
    for (Count i = 0; i < source_path.get_size(); i++) {
      if (source_path[i] == '/' || source_path[i] == '\\') {
        start = i + 1;
      }
    }

    Count end = source_path.get_size();
    for (Count i = start; i < source_path.get_size(); i++) {
      if (source_path[i] == '.') {
        end = i;
      }
    }

    return keep(source_path.slice(start, end - start));
  }

  static auto append_decimal(Dynamic::Bytes& output, Count value) -> void {
    Bits_8 digits[32];
    Count digit_count = 0;
    do {
      digits[digit_count++] = Bits_8('0' + value % 10);
      value /= 10;
    } while (value != 0);

    for (Count i = digit_count; i > 0; i--) {
      output.append(digits[i - 1]);
    }
  }

  auto build_header() -> Dynamic::Bytes {
    Dynamic::Bytes header;
    header.concat(
        "#pragma once\n\n"
        "#include \"perimortem/core/view/bytes.hpp\"\n\n"
        "namespace Ttx {\n\n"_view);
    library_compiler.append_header(header);
    header.concat("\n}  // namespace Ttx\n"_view);
    return header;
  }

  static auto write_file(View::Bytes path, View::Bytes bytes) -> Bool {
    File file;
    file.update_contents(bytes);
    return file.write(path);
  }

  auto keep(View::Bytes bytes) -> View::Bytes {
    Bits_8* copy = arena.allocate(bytes.get_size());
    Data::copy(copy, bytes.get_data(), bytes.get_size());
    return View::Bytes(copy, bytes.get_size());
  }

  static auto print_errors(const Resolver::Context& context) -> void {
    for (Count i = 0; i < context.get_errors().get_size(); i++) {
      auto error = context.get_errors()[i];
      print_bytes(stderr, error.get_source_path());
      const Ttx::Lexical::Token* token = error.get_start_token();
      if (token != nullptr) {
        fprintf(
            stderr, ":%u:%u", token->get_line(), token->get_column());
      }
      fprintf(stderr, ": ");
      print_bytes(stderr, error.get_message());
      fprintf(stderr, "\n");
    }
  }

  Allocator::Arena arena;
  Compiler::Library library_compiler;
  Dynamic::Bytes terminal_data;
  Count terminal_count = 0;
  Bool package = False;
};

Signed_32 main(Signed_32 argc, Signed_8** argv) {
  Diagnostics::Log::set_sink(Diagnostics::Log::console_sink);
  Diagnostics::Log::set_disable_header(True);

  constexpr Count max_argument_count = 256;
  Static::Vector<View::Bytes, max_argument_count> arguments;
  Count argument_count = Count(argc);
  if (argument_count > max_argument_count) {
    Diagnostics::Log::error(
        "puffer: too many command line arguments\n"_view,
        Diagnostics::Source());
    return 2;
  }

  for (Count i = 0; i < argument_count; i++) {
    arguments[i] = NullTerminated::to_view(argv[i]);
  }

  Puffer puffer;
  return puffer.run(arguments.slice(0, argument_count));
}
