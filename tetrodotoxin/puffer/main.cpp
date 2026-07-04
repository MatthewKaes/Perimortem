// Perimortem Engine
// Copyright © Matt Kaes

#include <stdio.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/args.hpp"
#include "perimortem/system/file.hpp"

#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"
#include "tetrodotoxin/resolution/source/record.hpp"
#include "tetrodotoxin/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Linker;
using namespace Tetrodotoxin::Resolution;

class Puffer {
 public:
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

    if (terminals.get_size() == 0) {
      fprintf(stderr, "puffer: no TTX roots were selected for output\n");
      return 1;
    }

    Tetrodotoxin::Linker::Linker linker;
    linker.add_section(Object::Section::Type::ReadOnly, terminal_data);
    for (Count i = 0; i < terminals.get_size(); i++) {
      linker.add_symbol(Object::Symbol::create_read_only(
          terminals[i].symbol_name, {terminals[i].offset, terminals[i].size}));
    }

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

    return 0;
  }

 private:
  class Terminal {
   public:
    View::Bytes symbol_name;
    Count offset = 0;
    Count size = 0;
  };

  using Configs = Managed::Map<View::Bytes, Args::Config>;

  static constexpr View::Bytes help_summary =
      "Compile TTX sources into terminal archives for Bazel."_view;

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
    }

    if (package && selected_sources == 0) {
      fprintf(stderr, "puffer: package mode needs a package.ttx source\n");
      return False;
    }

    return True;
  }

  auto add_terminal(Source::Record& record) -> void {
    Terminal terminal;
    terminal.symbol_name = build_symbol_name(record);
    terminal.offset = terminal_data.get_size();

    append_field("source"_view, record.get_source_path());
    append_field("import"_view, record.get_import_name());
    append_field("isa"_view, record.get_boot().get_isa());
    if (record.get_type() != nullptr) {
      append_field("type"_view, record.get_type()->get_name());
    }

    terminal.size = terminal_data.get_size() - terminal.offset;
    terminals.insert(terminal);
  }

  auto append_field(View::Bytes name, View::Bytes value) -> void {
    terminal_data.concat(name);
    terminal_data.append(':');
    terminal_data.append(' ');
    terminal_data.concat(value);
    terminal_data.append('\n');
  }

  auto build_symbol_name(Source::Record& record) -> View::Bytes {
    View::Bytes key = record.get_import_name().is_empty()
                          ? record.get_source_path()
                          : record.get_import_name();
    Dynamic::Bytes name;
    name.concat("puffer_terminal_"_view);
    append_hex(name, Hash(key).get_value());
    return keep(name.get_view());
  }

  static auto append_hex(Dynamic::Bytes& output, Bits_64 value) -> void {
    constexpr View::Bytes digits = "0123456789abcdef"_view;
    for (Signed_32 shift = 60; shift >= 0; shift -= 4) {
      output.append(digits[(value >> shift) & 0x0F]);
    }
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
    header.concat("#pragma once\n\n#include <stddef.h>\n\n"_view);
    for (Count i = 0; i < terminals.get_size(); i++) {
      header.concat("extern const unsigned char "_view);
      header.concat(terminals[i].symbol_name);
      header.concat("[];\nstatic constexpr size_t "_view);
      header.concat(terminals[i].symbol_name);
      header.concat("_size = "_view);
      append_decimal(header, terminals[i].size);
      header.concat(";\n\n"_view);
    }
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
  Dynamic::Bytes terminal_data;
  Dynamic::Vector<Terminal> terminals;
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
