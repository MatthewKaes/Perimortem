// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/fold.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Validation {

inline auto test_fold(
    const Tetrodotoxin::Library::Language::Model::Pack& source)
    -> Perimortem::Utility::Result<
        Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>,
        Tetrodotoxin::Library::Language::Expression::Error> {
  const Ttx::Concept::Abstract& answer =
      Tetrodotoxin::Library::Language::query_fold(source);
  if (!Ttx::Concept::Constant::prove(answer)) {
    return Perimortem::Core::Option<
        Tetrodotoxin::Library::Language::Model::Pack&>();
  }
  auto pack = Tetrodotoxin::Library::Language::Model::Pack::from(
      const_cast<Ttx::Concept::Abstract&>(answer));
  return pack ? Perimortem::Core::Option<
                    Tetrodotoxin::Library::Language::Model::Pack&>(*pack)
              : Perimortem::Core::Option<
                    Tetrodotoxin::Library::Language::Model::Pack&>();
}

inline auto concept_is_nonfoldable(
    const Perimortem::Utility::Result<
        Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>,
        Tetrodotoxin::Library::Language::Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Perimortem::Core::Option<
          Tetrodotoxin::Library::Language::Model::Pack&>& selected) {
        return selected ? False : True;
      },
      [](const Tetrodotoxin::Library::Language::Expression::Error&) {
        return False;
      });
}

// Direct semantic unit tests still use a real Library root. The temporary
// Cursor only opens that root in the supplied Arena. Every resulting identity
// remains owned by the Arena and is reached through the Monograph graph.
inline auto create_library_monograph(
    Perimortem::Memory::Allocator::Arena& arena,
    Tetrodotoxin::Library::Dialect& dialect)
    -> Tetrodotoxin::Library::Language::Monograph& {
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, {}, {});
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  return Tetrodotoxin::Library::Language::Monograph::create_authored(
      cursor.get_arena(), Ttx::Concept::Documentation::get_empty(),
      Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()), dialect, dialect);
}

inline auto resolve_library_flag(
    const Tetrodotoxin::Library::Language::Monograph& monograph)
    -> const Tetrodotoxin::Library::Language::Model::Types::Flag& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Flag&>(
      monograph.resolve_concept("Bool"_view));
}

inline auto resolve_library_unsigned(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Unsigned& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Unsigned&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_signed(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Signed& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Signed&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_real(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Real& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Real&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_type(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Type& {
  return static_cast<const Tetrodotoxin::Library::Language::Model::Type&>(
      monograph.resolve_concept(name));
}

}  // namespace Validation
