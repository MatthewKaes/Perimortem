// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/terminal/native/provider.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Terminal::Native {

// Toolchain carries the host inputs admitted by an Environment for one native
// request. Keeping this owner separate from Compiler lets Puffer describe the
// available compiler, headers, archives, and linker arguments without loading
// LLVM or any Terminal implementation into the host process.
class Toolchain : public Ttx::Concept::Abstract {
 public:
  Toolchain(
      Perimortem::Core::View::Bytes compiler,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          include_roots,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> archives,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          link_options)
      : compiler(compiler),
        include_roots(include_roots),
        archives(archives),
        link_options(link_options) {}

  TTX_NAME("Native toolchain"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_compiler() const -> Perimortem::Core::View::Bytes {
    return compiler;
  }
  constexpr auto get_include_roots() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return include_roots;
  }
  constexpr auto get_archives() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return archives;
  }
  constexpr auto get_link_options() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return link_options;
  }

 private:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

  Perimortem::Core::View::Bytes compiler;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> include_roots;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> archives;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> link_options;
};

}  // namespace Tetrodotoxin::Terminal::Native
