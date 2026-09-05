// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Tetrodotoxin::Environment {

// Sdk reads the installation-owned mapping from authored artifact locators to
// confined shared-library paths. Build keeps naming stable SDK artifacts while
// an assembled release remains free to choose its physical directory layout.
class Sdk {
 public:
  static auto open(const std::string& root) -> std::optional<Sdk>;

  auto select(const std::string& artifact) const -> std::optional<std::string>;

 private:
  struct Entry {
    std::string artifact;
    std::string relative_path;
  };

  Sdk(std::string root, std::vector<Entry> entries)
      : root(std::move(root)), entries(std::move(entries)) {}

  std::string root;
  std::vector<Entry> entries;
};

}  // namespace Tetrodotoxin::Environment
