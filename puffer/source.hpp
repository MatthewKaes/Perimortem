// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

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
      Tetrodotoxin::Package::Repository::Repository& terminal_repository,
      Perimortem::Core::Option<Tetrodotoxin::Package::Repository::Repository&>
          package_repository,
      Bool dump_graph,
      Bool generate_cxx)
      : source(source),
        terminal_root(terminal_root),
        terminal_repository(terminal_repository),
        package_repository(package_repository),
        dump_graph(dump_graph),
        generate_cxx(generate_cxx) {}

  auto run() const -> S32;

 private:
  Perimortem::Core::View::Bytes source;
  Perimortem::Core::View::Bytes terminal_root;
  Tetrodotoxin::Package::Repository::Repository& terminal_repository;
  Perimortem::Core::Option<Tetrodotoxin::Package::Repository::Repository&>
      package_repository;
  Bool dump_graph;
  Bool generate_cxx;
};

}  // namespace Puffer
