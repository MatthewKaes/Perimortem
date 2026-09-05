// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <utility>
#include <vector>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/package/repository/repository.hpp"

namespace Puffer {

// Source owns one command-line source transaction from graph construction
// through independent Terminal publication.
class Source {
 public:
  constexpr Source(
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes terminal_root,
      Perimortem::Core::View::Bytes sdk_root,
      Tetrodotoxin::Package::Repository::Repository& repository,
      std::vector<Perimortem::Core::View::Bytes> arguments)
      : source(source),
        terminal_root(terminal_root),
        sdk_root(sdk_root),
        repository(repository),
        arguments(std::move(arguments)) {}

  auto run() const -> S32;

 private:
  Perimortem::Core::View::Bytes source;
  Perimortem::Core::View::Bytes terminal_root;
  Perimortem::Core::View::Bytes sdk_root;
  Tetrodotoxin::Package::Repository::Repository& repository;
  std::vector<Perimortem::Core::View::Bytes> arguments;
};

}  // namespace Puffer
