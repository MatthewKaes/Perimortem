// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/command.hpp"

#include <cstdio>
#include <unistd.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/args.hpp"
#include "perimortem/system/file.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "puffer/lsp/methods.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/llvm/compiler.hpp"
#include "tetrodotoxin/library/llvm/products.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem;

static auto create_config(Memory::Allocator::Arena& arena)
    -> Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes> {
  Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes> variables(arena);
  variables.insert(
      "pipe"_view, "Run as an LSP server over the provided socket."_view);
  variables.insert("library"_view, "Compile one Library source."_view);
  variables.insert("backend"_view, "Select the Library backend."_view);
  variables.insert("target"_view, "Select the native target."_view);
  variables.insert("debug"_view, "Select none, line, or full debug data."_view);
  variables.insert("name"_view, "Set the source semantic name."_view);
  variables.insert("ir"_view, "Write the emitted LLVM IR."_view);
  variables.insert("object"_view, "Write the emitted ELF object."_view);
  variables.insert("header"_view, "Write the generated C header."_view);
  variables.insert(""_view, "Read one positional Library source."_view);
  return variables;
}

static auto value(const System::Args::Values& args, Core::View::Bytes name)
    -> Core::View::Bytes {
  auto entry = args.find(name);
  if (!entry) {
    return {};
  }

  Core::View::Vector<Core::View::Bytes> values = (*entry).value->get_view();
  return values.is_empty() ? Core::View::Bytes() : values[0];
}

static auto has_one(const System::Args::Values& args, Core::View::Bytes name)
    -> Bool {
  auto entry = args.find(name);
  return entry && (*entry).value->get_view().get_size() == 1;
}

static auto write_error(Core::View::Bytes message) -> void {
  constexpr auto newline = "\n"_view;

  fwrite(message.get_data(), 1, CppSize(message.get_size()), stderr);
  fwrite(newline.get_data(), 1, CppSize(newline.get_size()), stderr);
}

static auto run_lsp(Core::View::Bytes pipe_name) -> Signed_32 {
  if (pipe_name.is_empty() || pipe_name == "true"_view) {
    write_error("puffer: -pipe needs a socket path"_view);
    return 2;
  }

  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
  Core::Diagnostics::Log::set_disable_header(True);
  Puffer::Lsp::Executor executor;
  executor.execute(pipe_name);
  return 0;
}

static auto render_errors(const Ttx::Lexical::Errors& errors) -> void {
  Memory::Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    write_error(errors.render_message(arena, index));
    arena.reset();
  }
}

static auto create_stage_path(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes path) -> Core::View::Bytes {
  Memory::Managed::Bytes buffer(arena);
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << path << ".puffer."_view << Signed_64(getpid()) << ".tmp"_view;
  return buffer.get_view();
}

// File owns content I/O and diagnostics. Puffer adds product staging because a
// compiler request must finish every byte stream before exposing any requested
// path. POSIX has no atomic rename for three unrelated paths, so a rename
// failure can leave an earlier product published even though all staging
// writes completed first.
static auto publish(
    const Tetrodotoxin::Library::Llvm::Products& products,
    Core::View::Bytes ir_path,
    Core::View::Bytes object_path,
    Core::View::Bytes header_path) -> Bool {
  Memory::Allocator::Arena arena;
  Core::View::Bytes ir_stage = create_stage_path(arena, ir_path);
  Core::View::Bytes object_stage = create_stage_path(arena, object_path);
  Core::View::Bytes header_stage = create_stage_path(arena, header_path);
  Bool written = System::File::write(products.get_llvm_ir(), ir_stage) &&
                 System::File::write(products.get_object(), object_stage) &&
                 System::File::write(products.get_header(), header_stage);
  if (!written) {
    System::File::remove(ir_stage);
    System::File::remove(object_stage);
    System::File::remove(header_stage);
    write_error("puffer: could not stage every requested product"_view);
    return False;
  }

  if (!System::File::replace(ir_stage, ir_path) ||
      !System::File::replace(object_stage, object_path) ||
      !System::File::replace(header_stage, header_path)) {
    System::File::remove(ir_stage);
    System::File::remove(object_stage);
    System::File::remove(header_stage);
    write_error("puffer: could not publish every requested product"_view);
    return False;
  }

  return True;
}

static auto run_library(const System::Args::Values& args) -> Signed_32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
  Core::Diagnostics::Log::set_disable_header(True);

  constexpr Core::Static::Vector<Core::View::Bytes, 8> required = {{
    ""_view,
    "backend"_view,
    "target"_view,
    "debug"_view,
    "name"_view,
    "ir"_view,
    "object"_view,
    "header"_view,
  }};
  for (Core::View::Bytes name : required.get_view()) {
    if (!has_one(args, name)) {
      write_error(
          "puffer: Library mode requires one source, name, backend, target, "
          "debug mode, IR path, object path, and header path"_view);
      return 2;
    }
  }

  if (!has_one(args, "library"_view) ||
      value(args, "library"_view) != "true"_view ||
      value(args, "backend"_view) != "llvm"_view ||
      value(args, "target"_view) != "x86_64-sysv"_view) {
    write_error("puffer: use -library -backend=llvm -target=x86_64-sysv"_view);
    return 2;
  }

  Core::View::Bytes debug_name = value(args, "debug"_view);
  Tetrodotoxin::Library::Llvm::Debug::Level debug;
  if (debug_name == "none"_view) {
    debug = Tetrodotoxin::Library::Llvm::Debug::Level::None;
  } else if (debug_name == "line"_view) {
    debug = Tetrodotoxin::Library::Llvm::Debug::Level::Line;
  } else if (debug_name == "full"_view) {
    debug = Tetrodotoxin::Library::Llvm::Debug::Level::Full;
  } else {
    write_error("puffer: -debug must be none, line, or full"_view);
    return 2;
  }

  Core::View::Bytes source_path = value(args, ""_view);
  Core::View::Bytes ir_path = value(args, "ir"_view);
  Core::View::Bytes object_path = value(args, "object"_view);
  Core::View::Bytes header_path = value(args, "header"_view);
  if (ir_path == object_path || ir_path == header_path ||
      object_path == header_path) {
    write_error("puffer: product paths must be distinct"_view);
    return 2;
  }

  auto source = System::File::read(source_path);
  if (!source) {
    write_error("puffer: could not read the Library source"_view);
    return 2;
  }

  Tetrodotoxin::Environment::Workspace workspace;
  if (!workspace.install_dialect<Tetrodotoxin::Library::Dialect>(
          "Library"_view)) {
    write_error("puffer: could not install the Library dialect"_view);
    return 1;
  }

  Ttx::Lexical::Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, value(args, "name"_view), source_path, *source);
  if (!interpreted) {
    render_errors(errors);
    return 1;
  }

  auto monograph =
      interpreted->select<Tetrodotoxin::Library::Language::Monograph>();
  if (!monograph) {
    write_error("puffer: Workspace did not publish a Library source"_view);
    return 1;
  }

  Memory::Allocator::Arena product_arena;
  Tetrodotoxin::Library::Llvm::Request request(
      *monograph, errors, source_path, *source,
      Tetrodotoxin::Library::Llvm::Target::X86_64SysV, debug);
  Tetrodotoxin::Library::Llvm::Compiler compiler;
  Utility::Result<
      Tetrodotoxin::Library::Llvm::Products,
      Tetrodotoxin::Library::Llvm::Failure>
      result = compiler.compile(product_arena, request);
  return result.visit(
      [&](const Tetrodotoxin::Library::Llvm::Products& products) -> Signed_32 {
        return publish(products, ir_path, object_path, header_path) ? 0 : 1;
      },
      [&](const Tetrodotoxin::Library::Llvm::Failure& failure) -> Signed_32 {
        if (failure == Tetrodotoxin::Library::Llvm::Failure::SourceRejected) {
          render_errors(errors);
        }

        return 1;
      });
}

auto Puffer::Command::run() const -> Signed_32 {
  constexpr Count maximum_arguments = 256;
  if (Count(argument_count) > maximum_arguments) {
    write_error("puffer: too many command line arguments"_view);
    return 2;
  }

  Core::Static::Vector<Core::View::Bytes, maximum_arguments> command_line;
  for (Count index = 0; index < Count(argument_count); index++) {
    command_line[index] = Core::NullTerminated::to_view(argument_values[index]);
  }

  Memory::Allocator::Arena arena;
  Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes> config =
      create_config(arena);
  System::Args::Values args = System::Args::parse(
      arena, config, command_line.slice(0, Count(argument_count)));
  if (args.contains("help"_view)) {
    Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
    Core::Diagnostics::Log::set_disable_header(True);
    System::Args::log_help(
        arena,
        "Run the Tetrodotoxin language server or compile one Library source."_view,
        config, command_line.slice(0, Count(argument_count)));
    return 0;
  }

  Bool pipe = args.contains("pipe"_view);
  Bool library = args.contains("library"_view);
  if (pipe == library) {
    write_error("puffer: select exactly one of -pipe or -library"_view);
    return 2;
  }

  return pipe ? run_lsp(value(args, "pipe"_view)) : run_library(args);
}
