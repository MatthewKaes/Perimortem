// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/command.hpp"

#include <cstdio>
#include <limits.h>
#include <string>
#include <unistd.h>
#include <vector>

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

static auto lsp_value(Core::View::Bytes argument)
    -> Core::Option<Core::View::Bytes> {
  static constexpr Core::View::Bytes prefix = "-lsp="_view;
  if (argument.get_size() <= prefix.get_size() ||
      argument.slice(0, prefix.get_size()) != prefix) {
    return {};
  }
  return argument.slice(prefix.get_size());
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

static auto find_sdk_root(Memory::Allocator::Arena& arena)
    -> Core::View::Bytes {
  char executable[PATH_MAX];
  const ssize_t size =
      readlink("/proc/self/exe", executable, sizeof(executable));
  BAIL_IF(size <= 0 || size == sizeof(executable));
  std::string candidate(executable, size_t(size));
  for (Count depth = 0; depth < 8; ++depth) {
    const size_t separator = candidate.rfind('/');
    BAIL_IF(separator == std::string::npos);
    candidate.resize(separator);
    const std::string marker =
        candidate + "/packages/ttx/Perimortem.Memory/package.ttx";
    if (access(marker.c_str(), R_OK) == 0) {
      return arena.proxy(
          Core::View::Bytes(
              reinterpret_cast<const U8*>(candidate.data()), candidate.size()));
    }
  }
  return {};
}

auto Puffer::Command::run() const -> S32 {
  if (argument_count < 2) {
    write_error("puffer: provide one source or one -lsp pipe"_view);
    return 2;
  }

  Core::View::Bytes root_argument =
      Core::NullTerminated::to_view(argument_values[1]);
  auto lsp = lsp_value(root_argument);
  if (lsp && argument_count != 2) {
    write_error("puffer: -lsp accepts no source arguments"_view);
    return 2;
  }

  Core::View::Bytes source = lsp ? Core::View::Bytes() : root_argument;
  std::vector<Core::View::Bytes> arguments;
  arguments.reserve(argument_count > 2 ? size_t(argument_count - 2) : 0);
  for (S32 index = 2; index < argument_count; ++index) {
    arguments.push_back(Core::NullTerminated::to_view(argument_values[index]));
  }

  Memory::Allocator::Arena arena;
  Core::View::Bytes current_path = rooted(arena, "."_view);
  if (current_path.is_empty()) {
    write_error("puffer: working directory is invalid"_view);
    return 2;
  }
  if (!lsp) {
    source = rooted(arena, root_argument);
    if (source.is_empty()) {
      write_error("puffer: source path is invalid"_view);
      return 2;
    }
  }

  Core::View::Bytes sdk_root = find_sdk_root(arena);
  if (sdk_root.is_empty()) {
    write_error("puffer: installed SDK root could not be located"_view);
    return 2;
  }

  Memory::Managed::Bytes package_root(arena, sdk_root);
  package_root.concat("/packages/ttx"_view);

  auto repository = Tetrodotoxin::Package::Repository::Repository::create(
      arena, current_path,
      Core::Option<Core::View::Bytes>(package_root.get_view()));
  if (!repository) {
    write_error("puffer: repository could not be opened"_view);
    return 2;
  }

  if (lsp) {
    Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
    Core::Diagnostics::Log::set_disable_header(True);
    Puffer::Lsp::Executor executor(*repository);
    executor.execute(*lsp);
    return 0;
  }

  return Puffer::Source(
             source, current_path, sdk_root, *repository, std::move(arguments))
      .run();
}
