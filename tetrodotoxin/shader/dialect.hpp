// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/render/dialect.hpp"

namespace Tetrodotoxin::Shader {

// Dialect borrows the exact installed Library and Render dependencies available
// to Shader sources. Their Monographs remain neighboring Workspace meaning and
// are selected through ordinary graph routes rather than manufactured children.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(
      Perimortem::Core::View::Bytes name,
      Library::Dialect& library,
      Render::Dialect& render)
      : Tetrodotoxin::Language::Dialect(name),
        library(library),
        render(render) {}

  Dialect(Library::Dialect& library, Render::Dialect& render)
      : Dialect("Shader"_view, library, render) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  auto encode(const Ttx::Concept::Abstract& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> override;

  auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  constexpr auto get_library() const -> const Library::Dialect& {
    return library;
  }

  constexpr auto get_render() const -> const Render::Dialect& { return render; }

 private:
  Library::Dialect& library;
  Render::Dialect& render;
};

}  // namespace Tetrodotoxin::Shader
