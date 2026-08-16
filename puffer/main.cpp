// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/args.hpp"

#include "puffer/lsp/methods.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using Configs = Managed::Map<View::Bytes, View::Bytes>;

static constexpr View::Bytes help_summary =
    "Run the Tetrodotoxin language server over a socket."_view;

static auto create_args(Allocator::Arena& arena) -> Configs {
  Configs variables(arena);
  variables.insert(
      "pipe"_view, "Run as an LSP server over the provided socket."_view);
  return variables;
}

static auto arg_value(const Args::Values& args, View::Bytes name)
    -> View::Bytes {
  auto entry = args.find(name);
  if (!entry) {
    return View::Bytes();
  }

  View::Vector<View::Bytes> values = (*entry).value->get_view();
  return values.is_empty() ? View::Bytes() : values[0];
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

static auto run(View::Vector<View::Bytes> command_line) -> Signed_32 {
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

  return run_lsp(arg_value(options, "pipe"_view));
}

Signed_32 main(Signed_32 argc, char** argv) {
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

  return run(arguments.slice(0, argument_count));
}
