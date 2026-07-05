// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/args.hpp"
#include "perimortem/system/file.hpp"

#include "tetrodotoxin/compiler/library.hpp"
#include "tetrodotoxin/compiler/shader.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/puffer/lsp/methods.hpp"
#include "tetrodotoxin/puffer/lsp/rpc/executor.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer::Resolution;

class Main {
 public:
  Main() : library_compiler(arena), shader_compiler(arena) {}

  auto run(View::Vector<View::Bytes> command_line) -> Signed_32 {
    Allocator::Arena args_arena;
    Configs args_config = create_args(args_arena);
    Args::Values options = Args::parse(args_arena, args_config, command_line);
    if (options.contains("help"_view)) {
      Args::log_help(args_arena, help_summary, args_config, command_line);
      return 0;
    }

    if (options.is_empty()) {
      return 2;
    }

    if (options.contains("pipe"_view)) {
      return run_lsp(arg_value(options, "pipe"_view));
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
      log_error("puffer: no TTX roots were selected for output\n"_view);
      return 1;
    }

    Tetrodotoxin::Linker::Linker linker;
    library_compiler.add_to(linker);
    shader_compiler.add_to(linker);

    View::Bytes output_path = arg_value(options, "output"_view);
    Dynamic::Bytes archive = linker.build_library("ttx_terminal.o"_view);
    if (!write_file(output_path, archive.get_view())) {
      log_write_error("archive"_view, output_path);
      return 1;
    }

    View::Bytes header_path = arg_value(options, "header"_view);
    Dynamic::Bytes header = build_header();
    if (!write_file(header_path, header.get_view())) {
      log_write_error("header"_view, header_path);
      return 1;
    }

    View::Bytes puffer_buffer_path = arg_value(options, "puffer"_view);
    if (!write_file(puffer_buffer_path, puffer_buffer.get_view())) {
      log_write_error("Puffer Buffer"_view, puffer_buffer_path);
      return 1;
    }

    return 0;
  }

 private:
  using Configs = Managed::Map<View::Bytes, View::Bytes>;

  static constexpr View::Bytes help_summary =
      "Compile TTX sources into archives and Puffer Buffers for Bazel."_view;

  static auto log_error(View::Bytes message) -> void {
    Diagnostics::Log::error(message, Diagnostics::Source());
  }

  static auto log_write_error(View::Bytes artifact, View::Bytes path) -> void {
    Diagnostics::Log::Message<512> error_message(
        Diagnostics::Log::Level::Error, Diagnostics::Source());
    error_message << "puffer: failed to write "_view << artifact << ' ' << path
                  << '\n';
  }

  static auto create_args(Allocator::Arena& arena) -> Configs {
    Configs variables(arena);
    variables.insert(
        "library"_view, "Compile source roots as TTX library terminals."_view);
    variables.insert(
        "package"_view,
        "Compile package manifests as TTX package terminals."_view);
    variables.insert("output"_view, "Write the output static archive."_view);
    variables.insert("header"_view, "Write the generated C++ header."_view);
    variables.insert("puffer"_view, "Write the Puffer Buffer output."_view);
    variables.insert(
        "dep"_view, "Make a dependency source visible while loading."_view);
    variables.insert("source"_view, "TTX source roots to compile."_view);
    variables.insert(
        "pipe"_view, "Run as an LSP server over the provided socket."_view);
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
    Bool library = args.contains("library"_view);
    package = args.contains("package"_view);
    if (library == package) {
      Diagnostics::Log::error(
          "puffer: select exactly one of -library or -package\n"_view,
          Diagnostics::Source());
      return False;
    }

    return require_arg(args, "output"_view) &&
           require_arg(args, "header"_view) &&
           require_arg(args, "puffer"_view) && require_arg(args, "source"_view);
  }

  static auto require_arg(const Args::Values& args, View::Bytes name) -> Bool {
    if (args.contains(name)) {
      return True;
    }

    Diagnostics::Log::Message<256> error_message(
        Diagnostics::Log::Level::Error, Diagnostics::Source());
    error_message << "missing required arg"_view << ' ' << name << '\n';
    return False;
  }

  static auto run_lsp(View::Bytes pipe_name) -> Signed_32 {
    if (pipe_name.is_empty() || pipe_name == "true"_view) {
      Diagnostics::Log::error(
          "puffer: -pipe needs a socket path\n"_view, Diagnostics::Source());
      return 2;
    }

    Diagnostics::Log::info("puffer: starting LSP server"_view);
    Puffer::Lsp::Rpc::Executor executor;
    Puffer::Lsp::register_methods(executor);
    executor.execute(pipe_name);
    return 0;
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
    View::Vector<View::Bytes> dependencies = arg_values(args, "dep"_view);
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
    View::Vector<View::Bytes> sources = arg_values(args, "source"_view);
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

      if (package) {
        if (!add_package_terminals(resolver, *record)) {
          return False;
        }
      } else {
        add_terminal(*record);
        if (!lower_record(*record)) {
          return False;
        }
      }
    }

    if (package && selected_sources == 0) {
      log_error("puffer: package mode needs a package.ttx source\n"_view);
      return False;
    }

    return True;
  }

  auto add_package_terminals(Resolver& resolver, Source::Record& root) -> Bool {
    Dynamic::Vector<Source::Record*> records;
    resolver.collect_reachable(root, records);
    for (Count i = 0; i < records.get_size(); i++) {
      add_terminal(*records[i]);
      if (!lower_record(*records[i])) {
        return False;
      }
    }

    return True;
  }

  auto lower_record(Source::Record& record) -> Bool {
    if (record.get_type() == nullptr) {
      return True;
    }

    View::Bytes module = build_module_name(record.get_source_path());
    View::Bytes isa = record.get_boot().get_isa();
    if (!package && isa == "Library"_view &&
        !library_compiler.lower(module, *record.get_type())) {
      Diagnostics::Log::Message<512> error_message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      error_message << "puffer: "_view << library_compiler.get_error() << '\n';
      return False;
    }

    if (isa == "Shader"_view &&
        !shader_compiler.lower(module, *record.get_type())) {
      Diagnostics::Log::Message<512> error_message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      error_message << "puffer: "_view << shader_compiler.get_error() << '\n';
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
    puffer_buffer.concat(name);
    puffer_buffer.append(':');
    puffer_buffer.append(' ');
    puffer_buffer.concat(value);
    puffer_buffer.append('\n');
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

  auto append_member_fact(View::Bytes owner, const Ttx::Type::Member& member)
      -> void {
    puffer_buffer.concat("member: "_view);
    puffer_buffer.concat(owner);
    puffer_buffer.concat("."_view);
    puffer_buffer.concat(
        member.get_name().is_empty() ? "_"_view : member.get_name());
    puffer_buffer.concat(" "_view);
    puffer_buffer.concat(type_name(member.get_type()));
    puffer_buffer.append('\n');
  }

  auto append_function_fact(
      View::Bytes owner,
      const Ttx::Type::Function& function) -> void {
    puffer_buffer.concat("function: "_view);
    puffer_buffer.concat(owner);
    puffer_buffer.concat("->"_view);
    puffer_buffer.concat(function.get_name());
    puffer_buffer.concat(" params="_view);
    append_decimal(puffer_buffer, function.get_parameters().get_size());
    puffer_buffer.concat(" result="_view);
    append_decimal(puffer_buffer, function.get_result().get_size());
    puffer_buffer.concat(" blocks="_view);
    append_decimal(puffer_buffer, function.get_blocks().get_size());
    puffer_buffer.append('\n');
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
      Diagnostics::Log::Message<2048> error_message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      error_message << error.get_source_path();
      const Ttx::Lexical::Token* token = error.get_start_token();
      if (token != nullptr) {
        error_message << ':' << token->get_line() << ':' << token->get_column();
      }
      error_message << ": "_view << error.get_message() << '\n';
    }
  }

  Allocator::Arena arena;
  Compiler::Library library_compiler;
  Compiler::Shader shader_compiler;
  Dynamic::Bytes puffer_buffer;
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

  Main main;
  return main.run(arguments.slice(0, argument_count));
}
