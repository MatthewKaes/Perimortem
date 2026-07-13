// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/args.hpp"
#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/puffer/compiler.hpp"
#include "tetrodotoxin/puffer/lsp/methods.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

class Main {
 public:
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

    Puffer::Compiler compiler(mode, arg_value(options, "name"_view));
    View::Vector<View::Bytes> dependencies = arg_values(options, "dep"_view);
    for (Count i = 0; i < dependencies.get_size(); i++) {
      if (!compiler.add_dependency(dependencies[i])) {
        print_errors(compiler.get_errors());
        return 1;
      }
    }

    View::Vector<View::Bytes> sources = arg_values(options, "source"_view);
    for (Count i = 0; i < sources.get_size(); i++) {
      if (!compiler.add_source(sources[i])) {
        print_errors(compiler.get_errors());
        return 1;
      }
    }

    View::Bytes output_path = arg_value(options, "output"_view);
    Path output(output_path);
    View::Bytes file = output.get_file();
    View::Bytes extension = output.get_extension();
    Dynamic::Bytes object_name(
        file.slice(0, file.get_size() - extension.get_size()));
    object_name.concat(".o"_view);
    Dynamic::Bytes archive;
    Dynamic::Bytes header;
    Dynamic::Bytes puffer_buffer;
    if (!compiler.build(object_name, archive, header, puffer_buffer)) {
      print_errors(compiler.get_errors());
      return 1;
    }

    if (!write_file(output_path, archive)) {
      log_write_error("archive"_view, output_path);
      return 1;
    }

    View::Bytes header_path = arg_value(options, "header"_view);
    if (!write_file(header_path, header)) {
      log_write_error("header"_view, header_path);
      return 1;
    }

    View::Bytes puffer_buffer_path = arg_value(options, "puffer"_view);
    if (!write_file(puffer_buffer_path, puffer_buffer)) {
      log_write_error("Puffer Buffer"_view, puffer_buffer_path);
      return 1;
    }

    return 0;
  }

 private:
  using Configs = Managed::Map<View::Bytes, View::Bytes>;

  static constexpr View::Bytes help_summary =
      "Compile TTX sources into archives and Puffer Buffers for Bazel."_view;
  // Keep terminal source diagnostics visually aligned with the bundled TTX
  // editor theme: red for the failing span, muted brown for context.
  static constexpr View::Bytes diagnostic_reset = "\x1b[0m"_view;
  static constexpr View::Bytes diagnostic_error = "\x1b[38;2;221;109;114m"_view;
  static constexpr View::Bytes diagnostic_source =
      "\x1b[38;2;216;216;216m"_view;
  static constexpr View::Bytes diagnostic_hint = "\x1b[38;2;120;112;101m"_view;

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
        "name"_view, "Resolved package name for -package outputs."_view);
    variables.insert(
        "dep"_view,
        "Make a dependency Puffer Buffer visible while loading."_view);
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
    Bool package = args.contains("package"_view);
    if (library == package) {
      Diagnostics::Log::error(
          "puffer: select exactly one of -library or -package\n"_view,
          Diagnostics::Source());
      return False;
    }

    mode = package ? Puffer::Compiler::Mode::Package
                   : Puffer::Compiler::Mode::Library;
    return require_arg(args, "output"_view) &&
           require_arg(args, "header"_view) &&
           require_arg(args, "puffer"_view) &&
           (!package || require_arg(args, "name"_view)) &&
           require_arg(args, "source"_view);
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
    Puffer::Lsp::Executor executor;
    executor.execute(pipe_name);
    return 0;
  }

  static auto write_file(View::Bytes path, View::Bytes bytes) -> Bool {
    return File::write(bytes, path);
  }

  static auto source_line(View::Bytes source, Count line) -> View::Bytes {
    Count current_line = 1;
    Count line_start = 0;
    for (Count i = 0; i < source.get_size(); i++) {
      if (source[i] != '\n') {
        continue;
      }

      if (current_line == line) {
        return source.slice(line_start, i - line_start);
      }

      current_line++;
      line_start = i + 1;
    }

    return current_line == line
               ? source.slice(line_start, source.get_size() - line_start)
               : View::Bytes();
  }

  static auto highlight_width(
      const Ttx::Lexical::Token& start,
      const Ttx::Lexical::Token* end,
      View::Bytes line) -> Count {
    Count start_column = start.get_column();
    Count line_end_column = line.get_size() + 1;
    if (end == nullptr || end->get_line() != start.get_line()) {
      return line_end_column > start_column ? line_end_column - start_column
                                            : Count(1);
    }

    Count end_column = Count(end->get_column()) + end->get_text().get_size();
    return end_column > start_column ? end_column - start_column : Count(1);
  }

  template <Count size>
  static auto append_hint(
      Diagnostics::Log::Message<size>& error_message,
      const Ttx::Lexical::Errors::Error& error) -> void {
    if (error.get_hint().is_empty()) {
      return;
    }

    error_message << "  "_view << diagnostic_hint << "hint: "_view
                  << error.get_hint() << diagnostic_reset << '\n';
  }

  template <Count size>
  static auto append_source_range(
      Diagnostics::Log::Message<size>& error_message,
      const Ttx::Lexical::Errors::Error& error) -> void {
    const Ttx::Lexical::Token* start = error.get_start_token();
    if (start == nullptr || error.get_source().is_empty()) {
      append_hint(error_message, error);
      return;
    }

    View::Bytes line = source_line(error.get_source(), start->get_line());
    if (line.is_empty()) {
      append_hint(error_message, error);
      return;
    }

    error_message << "  "_view << diagnostic_source << line << diagnostic_reset
                  << '\n'
                  << "  "_view;
    for (Count i = 1; i < start->get_column(); i++) {
      error_message << ' ';
    }

    Count width = highlight_width(*start, error.get_end_token(), line);
    error_message << diagnostic_error << '^';
    for (Count i = 1; i < width; i++) {
      error_message << '~';
    }

    error_message << diagnostic_reset << '\n';
    append_hint(error_message, error);
  }

  static auto print_errors(View::Vector<Ttx::Lexical::Errors::Error> errors)
      -> void {
    for (Count i = 0; i < errors.get_size(); i++) {
      Diagnostics::Log::Message<4096> error_message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      if (!errors[i].get_source_path().is_empty()) {
        error_message << errors[i].get_source_path();
        const Ttx::Lexical::Token* token = errors[i].get_start_token();
        if (token != nullptr) {
          error_message << ':' << token->get_line() << ':'
                        << token->get_column();
        }

        error_message << ": "_view;
      } else {
        error_message << "puffer: "_view;
      }

      error_message << "error: "_view << diagnostic_error
                    << errors[i].get_message() << diagnostic_reset << '\n';
      append_source_range(error_message, errors[i]);
      if (i + 1 < errors.get_size()) {
        error_message << '\n';
      }
    }
  }

  Puffer::Compiler::Mode mode = Puffer::Compiler::Mode::Library;
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
