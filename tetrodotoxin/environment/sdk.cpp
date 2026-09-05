// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/sdk.hpp"

#include <string_view>
#include <utility>

#include "perimortem/system/file.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static auto valid_relative(std::string_view path) -> bool {
  if (path.empty() || path.front() == '/' || path.front() == '\\') {
    return false;
  }
  size_t start = 0;
  for (size_t index = 0; index <= path.size(); ++index) {
    if (index != path.size() && path[index] != '/') {
      if (path[index] == '\\' || path[index] == '\0') {
        return false;
      }
      continue;
    }
    const std::string_view segment = path.substr(start, index - start);
    if (segment.empty() || segment == "." || segment == "..") {
      return false;
    }
    start = index + 1;
  }
  return true;
}

static auto append_path(const std::string& root, const std::string& relative)
    -> std::string {
  return root.empty() || root.back() == '/' ? root + relative
                                            : root + "/" + relative;
}

auto Environment::Sdk::open(const std::string& root) -> std::optional<Sdk> {
  const std::string manifest = append_path(root, "sdk/providers.manifest");
  auto contents = System::File::read(
      Core::View::Bytes(
          reinterpret_cast<const uint8_t*>(manifest.data()), manifest.size()));
  if (!contents) {
    return std::nullopt;
  }
  const Core::View::Bytes bytes = contents->get_view();
  const std::string_view source(
      reinterpret_cast<const char*>(bytes.get_data()), bytes.get_size());
  const std::string_view header = "ttx.sdk 1";
  if (!source.starts_with(header) ||
      (source.size() != header.size() && source[header.size()] != '\n')) {
    return std::nullopt;
  }

  std::vector<Entry> entries;
  size_t position =
      source.size() == header.size() ? source.size() : header.size() + 1;
  while (position < source.size()) {
    const size_t ending = source.find('\n', position);
    const std::string_view line = source.substr(
        position,
        (ending == std::string_view::npos ? source.size() : ending) - position);
    position = ending == std::string_view::npos ? source.size() : ending + 1;
    if (line.empty() || line.front() == '#') {
      continue;
    }
    const size_t separator = line.find(' ');
    if (separator == std::string_view::npos || separator == 0 ||
        separator + 1 == line.size()) {
      return std::nullopt;
    }
    const std::string artifact(line.substr(0, separator));
    const std::string relative(line.substr(separator + 1));
    if (!valid_relative(relative)) {
      return std::nullopt;
    }
    for (const Entry& existing : entries) {
      if (existing.artifact == artifact) {
        return std::nullopt;
      }
    }
    entries.push_back({artifact, relative});
  }
  return entries.empty() ? std::optional<Sdk>()
                         : std::optional<Sdk>(Sdk(root, std::move(entries)));
}

auto Environment::Sdk::select(const std::string& artifact) const
    -> std::optional<std::string> {
  for (const Entry& entry : entries) {
    if (entry.artifact == artifact) {
      return append_path(root, entry.relative_path);
    }
  }
  return std::nullopt;
}
