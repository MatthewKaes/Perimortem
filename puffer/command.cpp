// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/command.hpp"

#include <cstdio>
#include <limits.h>
#include <unistd.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/path.hpp"

#include "puffer/lsp/methods.hpp"
#include "puffer/source.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"

using namespace Perimortem;

static auto write_error(Core::View::Bytes message) -> void {
  fwrite(message.get_data(), 1, CppSize(message.get_size()), stderr);
  fwrite("\n", 1, 1, stderr);
}

static auto option_value(Core::View::Bytes argument, Core::View::Bytes name)
    -> Core::Option<Core::View::Bytes> {
  if (argument.get_size() <= name.get_size() + 2 || argument[0] != '-' ||
      argument.slice(1, name.get_size()) != name ||
      argument[name.get_size() + 1] != '=') {
    return {};
  }
  return argument.slice(name.get_size() + 2);
}

static auto rooted(Memory::Allocator::Arena& arena, Core::View::Bytes path)
    -> Core::View::Bytes {
  System::Path selected(path);
  if (selected.is_rooted()) {
    auto normalized = System::Path::normalize(arena, path);
    return normalized ? *normalized : Core::View::Bytes();
  }

  char current[PATH_MAX];
  BAIL_IF(getcwd(current, sizeof(current)) == nullptr);
  Memory::Managed::Bytes combined(
      arena, Core::NullTerminated::to_view(current));
  combined.append('/');
  combined.concat(path);
  auto normalized = System::Path::normalize(arena, combined.get_view());
  return normalized ? *normalized : Core::View::Bytes();
}

auto Puffer::Command::run() const -> S32 {
  constexpr Count maximum_arguments = 16;
  if (argument_count < 1 || Count(argument_count) > maximum_arguments) {
    write_error("puffer: invalid argument count"_view);
    return 2;
  }

  Core::View::Bytes source;
  Core::View::Bytes lsp;
  Core::View::Bytes package_root;
  Core::View::Bytes terminal_root = "."_view;
  Bool dump_graph = False;
  Bool generate_cxx = False;
  Bool has_lsp = False;
  Bool has_package_repository = False;
  Bool has_terminal_repository = False;
  for (Count index = 1; index < Count(argument_count); index++) {
    Core::View::Bytes argument =
        Core::NullTerminated::to_view(argument_values[index]);
    auto selected_lsp = option_value(argument, "lsp"_view);
    auto selected_packages = option_value(argument, "package_repository"_view);
    auto selected_terminals =
        option_value(argument, "terminal_repository"_view);
    if (selected_lsp) {
      if (has_lsp) {
        write_error("puffer: -lsp may be provided once"_view);
        return 2;
      }
      has_lsp = True;
      lsp = *selected_lsp;
    } else if (selected_packages) {
      if (has_package_repository) {
        write_error("puffer: -package_repository may be provided once"_view);
        return 2;
      }
      has_package_repository = True;
      package_root = *selected_packages;
    } else if (selected_terminals) {
      if (has_terminal_repository) {
        write_error("puffer: -terminal_repository may be provided once"_view);
        return 2;
      }
      has_terminal_repository = True;
      terminal_root = *selected_terminals;
    } else if (argument == "-dump_graph"_view) {
      if (dump_graph) {
        write_error("puffer: -dump_graph may be provided once"_view);
        return 2;
      }
      dump_graph = True;
    } else if (argument == "-generate_cxx"_view) {
      if (generate_cxx) {
        write_error("puffer: -generate_cxx may be provided once"_view);
        return 2;
      }
      generate_cxx = True;
    } else if (!argument.is_empty() && argument[0] != '-') {
      if (!source.is_empty()) {
        write_error("puffer: exactly one source is accepted"_view);
        return 2;
      }
      source = argument;
    } else {
      write_error("puffer: unsupported argument"_view);
      return 2;
    }
  }

  if ((has_lsp && (!source.is_empty() || dump_graph || generate_cxx)) ||
      (!has_lsp && source.is_empty())) {
    write_error("puffer: provide one source or one -lsp pipe"_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
  Core::View::Bytes terminal_path = rooted(arena, terminal_root);
  Core::View::Bytes package_path = package_root.is_empty()
                                       ? Core::View::Bytes()
                                       : rooted(arena, package_root);
  if (terminal_path.is_empty() ||
      (!package_root.is_empty() && package_path.is_empty())) {
    write_error("puffer: repository path is invalid"_view);
    return 2;
  }

  auto repository = Tetrodotoxin::Package::Repository::Repository::create(
      arena, terminal_path,
      package_path.is_empty() ? Core::Option<Core::View::Bytes>()
                              : Core::Option<Core::View::Bytes>(package_path));
  if (!repository) {
    write_error("puffer: repository could not be opened"_view);
    return 2;
  }

  if (!lsp.is_empty()) {
    Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
    Core::Diagnostics::Log::set_disable_header(True);
    Puffer::Lsp::Executor executor(*repository);
    executor.execute(lsp);
    return 0;
  }

  return Puffer::Source(
             source, terminal_path, *repository, dump_graph, generate_cxx)
      .run();
}
