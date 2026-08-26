// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

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

#include "puffer/application.hpp"
#include "puffer/lsp/methods.hpp"
#include "puffer/package.hpp"
#include "ttx/lexical/formatter.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem;

static auto create_config(Memory::Allocator::Arena& arena)
    -> Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes> {
  Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes> variables(arena);
  variables.insert(
      "pipe"_view, "Run as an LSP server over the provided socket."_view);
  variables.insert(
      "packages-root"_view,
      "Select the standard source Package root for LSP sessions."_view);
  variables.insert("format"_view, "Format each positional TTX source."_view);
  variables.insert("package"_view, "Compile one Package."_view);
  variables.insert("application"_view, "Compile one App process entry."_view);
  variables.insert("manifest"_view, "Read one Package manifest."_view);
  variables.insert("complete"_view, "Write the Complete Package Archive."_view);
  variables.insert("contract"_view, "Write the Contract Package Archive."_view);
  variables.insert("dep"_view, "Read one dependency Contract Archive."_view);
  variables.insert(
      "dep-abi"_view, "Read one dependency native ABI Manifest."_view);
  variables.insert(
      "abi-manifest"_view, "Read or write one native ABI Manifest."_view);
  variables.insert(
      "native-provider"_view,
      "Select one logical provider for a native import."_view);
  variables.insert("unit"_view, "Compile one declared Package member."_view);
  variables.insert(
      "product-unit"_view,
      "Write sibling Terminal products for one Package member."_view);
  variables.insert(
      "artifact"_view, "Select the native artifact identity."_view);
  variables.insert(
      "spirv-target"_view, "Select the SPIR V validation environment."_view);
  variables.insert(
      "graphics-placement"_view,
      "Select one graphics Placement2D requirement route."_view);
  variables.insert(
      "graphics-type"_view, "Select one hosted graphics Type route."_view);
  variables.insert(
      "graphics-placement-provider"_view,
      "Select one native graphics Placement2D provider."_view);
  variables.insert(
      "graphics-children-provider"_view,
      "Select one native graphics Children2D provider."_view);
  variables.insert(
      "graphics-drawable-provider"_view,
      "Select one native graphics Drawable2D provider."_view);
  variables.insert("app-member"_view, "Select the Package App member."_view);
  variables.insert(
      "version"_view, "Set the Package `<major>.<minor>` version."_view);
  variables.insert("debug"_view, "Select none, line, or full debug data."_view);
  variables.insert("name"_view, "Set the source semantic name."_view);
  variables.insert(
      "source"_view, "Write the generated native entry source."_view);
  variables.insert("ir"_view, "Write the emitted LLVM IR."_view);
  variables.insert("object"_view, "Write the emitted ELF object."_view);
  variables.insert(
      "resources-object"_view,
      "Write the Package owned immutable Resource object."_view);
  variables.insert("header"_view, "Write the generated C header."_view);
  variables.insert(
      "cpp-header"_view, "Write the generated C++ Package header."_view);
  variables.insert(
      "cpp-source"_view, "Write the generated C++ Package source."_view);
  variables.insert(
      "cpp-include"_view, "Set the generated C++ include path."_view);
  variables.insert("c-include"_view, "Set the generated C include path."_view);
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

static auto run_lsp(
    Core::View::Bytes pipe_name,
    Core::View::Bytes packages_root) -> S32 {
  if (pipe_name.is_empty() || pipe_name == "true"_view) {
    write_error("puffer: -pipe needs a socket path"_view);
    return 2;
  }

  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
  Core::Diagnostics::Log::set_disable_header(True);
  Puffer::Lsp::Executor executor(packages_root);
  executor.execute(pipe_name);
  return 0;
}

static auto create_stage_path(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes path) -> Core::View::Bytes {
  Memory::Managed::Bytes buffer(arena);
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << path << ".puffer."_view << S64(getpid()) << ".tmp"_view;
  return buffer.get_view();
}

static auto run_format(const System::Args::Values& args) -> S32 {
  auto sources = args.find(""_view);
  if (!has_one(args, "format"_view) ||
      value(args, "format"_view) != "true"_view || !sources ||
      (*sources).value->is_empty()) {
    write_error("puffer: -format needs at least one TTX source"_view);
    return 2;
  }

  for (Core::View::Bytes path : (*sources).value->get_view()) {
    auto source = System::File::read(path);
    if (!source) {
      write_error("puffer: could not read a TTX source for formatting"_view);
      return 1;
    }

    Memory::Allocator::Arena transaction;
    Ttx::Lexical::Tokenizer tokenizer(transaction, source->get_view(), path);
    Memory::Dynamic::Bytes formatted =
        Ttx::Lexical::Formatter(tokenizer).format();
    Core::View::Bytes stage = create_stage_path(transaction, path);
    Bool written = System::File::write(formatted.get_view(), stage);
    if (!written) {
      System::File::remove(stage);
      write_error("puffer: could not stage a formatted TTX source"_view);
      return 1;
    }

    Bool published = System::File::replace(stage, path);
    if (!published) {
      System::File::remove(stage);
      write_error("puffer: could not publish a formatted TTX source"_view);
      return 1;
    }
  }

  return 0;
}

auto Puffer::Command::run() const -> S32 {
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
        "Run the language server, format TTX sources, or build a Package."_view,
        config, command_line.slice(0, Count(argument_count)));
    return 0;
  }

  Bool pipe = args.contains("pipe"_view);
  Bool format = args.contains("format"_view);
  Bool package = args.contains("package"_view);
  Bool application = args.contains("application"_view);
  Count modes = Count(pipe.value) + Count(format.value) + Count(package.value) +
                Count(application.value);
  if (modes != 1) {
    write_error("puffer: select exactly one execution mode"_view);
    return 2;
  }

  if (pipe) {
    return run_lsp(value(args, "pipe"_view), value(args, "packages-root"_view));
  } else if (format) {
    return run_format(args);
  } else if (package) {
    return Puffer::Package(args).run();
  } else if (application) {
    return Puffer::Application(args).run();
  }
  return 2;
}
