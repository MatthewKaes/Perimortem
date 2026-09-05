// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/shader/language/bridge.hpp"
#include "tetrodotoxin/shader/language/program.hpp"

namespace Tetrodotoxin::Shader::Language {

// Monograph owns the Shader relationships authored by one source and one real
// Library child containing its executable meaning. The child exists because
// Shader bodies directly author Library Functions, Fields, Types, expressions,
// and Flow. Render contracts remain neighboring Workspace identities.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library) -> Monograph&;

  auto retain_program(Program& program) -> Bool;
  auto retain_bridge(Bridge& bridge) -> Bool;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored() -> Bool override;
  auto finalize_restored() -> Bool override;

  auto get_layer(const Ttx::Concept::Abstract& requested) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Language::Monograph&> override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool override {
    return library.retain_import(description, associations);
  }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Language::Import*> override {
    return library.get_imports();
  }

  constexpr auto get_programs() const
      -> Perimortem::Core::View::Vector<Program*> {
    return programs;
  }

  constexpr auto get_bridges() const
      -> Perimortem::Core::View::Vector<Bridge*> {
    return bridges;
  }

  TTX_NAME("Shader"_view);

  constexpr auto edit_library() -> Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto get_library() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto is_finalized() const -> Bool { return finalized; }

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library)
      : Tetrodotoxin::Language::Monograph(
            domain,
            language,
            documentation,
            context),
        library(library),
        programs(domain),
        bridges(domain) {}

  Tetrodotoxin::Library::Language::Monograph& library;
  Perimortem::Memory::Managed::Vector<Program*> programs;
  Perimortem::Memory::Managed::Vector<Bridge*> bridges;
  Bool linked = False;
  Bool finalized = False;
};

}  // namespace Tetrodotoxin::Shader::Language
