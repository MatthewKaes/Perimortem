// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/lsp/server/methods.hpp"
#include "tetrodotoxin/lsp/server/rpc/executor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lsp;

auto main(int argc, char* argv[]) -> int {
  Diagnostics::Log::set_sink(Diagnostics::Log::color_sink);
  Diagnostics::Log::info("~~ TTX Lang Server ~~"_view);
  Diagnostics::Log::info("[Lsp API Version: 3.17]"_view);

  constexpr auto pipe_prefix = "--pipe="_view;
  View::Bytes pipe_name;

  for (Signed_32 i = 1; i < argc; i++) {
    View::Bytes argument = NullTerminated::to_view(argv[i]);

    if (argument.slice(0, pipe_prefix.get_size()) == pipe_prefix) {
      pipe_name = argument.slice(pipe_prefix.get_size());
    }
  }

  if (pipe_name.is_empty()) {
    Diagnostics::Log::error(
        "No `--pipe=` provided, closing language server."_view);
    return -1;
  }

  Diagnostics::Log::info("Creating RPC Server using pipe..."_view);
  Server::Rpc::Executor executor;
  Server::register_methods(executor);

  Diagnostics::Log::info("Starting RPC..."_view);
  executor.execute(pipe_name);

  return 0;
}
