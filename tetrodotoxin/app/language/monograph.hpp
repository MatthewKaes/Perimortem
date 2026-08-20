// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/app/language/program.hpp"
#include "tetrodotoxin/app/language/runtime.hpp"
#include "tetrodotoxin/language/monograph.hpp"

namespace Tetrodotoxin::App::Language {

// Monograph owns one completed application policy while its Runtime and
// Program identities remain in the same source transaction Arena.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Runtime& runtime,
      Program& program) -> Monograph&;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_restored() -> Bool override;
  auto finalize_restored() -> Bool override;

  TTX_NAME("App"_view);

  constexpr auto get_runtime() const -> const Runtime& { return runtime; }
  constexpr auto get_program() const -> const Program& { return program; }

 private:
  constexpr Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Runtime& runtime,
      Program& program)
      : Tetrodotoxin::Language::Monograph(
            arena,
            language,
            documentation,
            context),
        runtime(runtime),
        program(program) {}

  Runtime& runtime;
  Program& program;
};

}  // namespace Tetrodotoxin::App::Language
