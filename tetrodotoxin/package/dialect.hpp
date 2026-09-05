// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Tetrodotoxin::Package {

struct RequiredDialect {
  Perimortem::Core::View::Bytes package;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes route;
};

// Dialect parses one restricted Library export surface. Workspace owns the
// imported source graph, completion, and lifetime.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:

  Dialect(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Library::Dialect& library)
      : Tetrodotoxin::Language::Dialect(name), library(library) {}

  Dialect(Tetrodotoxin::Library::Dialect& library)
      : Dialect("Package"_view, library) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  // Direct Package invocation performs this same lexical read before it loads
  // providers. The operation consumes only the host-independent requires
  // preamble and leaves Package interpretation to the resulting Toolchain.
  static auto parse_requirements(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<
          Perimortem::Memory::Managed::Vector<RequiredDialect>>;

  void produce(
      ttx_context context,
      Perimortem::Memory::Allocator::Arena& arena,
      tetrodotoxin_workspace_view workspace,
      const Tetrodotoxin::Language::Monograph& monograph,
      tetrodotoxin_production_result result) const override;

  constexpr auto get_library() const -> Tetrodotoxin::Library::Dialect& {
    return library;
  }

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Package
