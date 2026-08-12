// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language {

// ArgumentPack is the identity-free Layout for one authored `(...)` value
// pack. It retains the real Expressions in source order and keeps labels as
// lexical facts, so named fitting never manufactures Alias objects merely to
// attach another name to an Expression.
class ArgumentPack : public Ttx::Concept::Layout {
 public:
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<ArgumentPack&>;

  static auto create_empty(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Anchor anchor) -> ArgumentPack&;

  ArgumentPack(const ArgumentPack&) = delete;
  ArgumentPack(ArgumentPack&&) = delete;
  auto operator=(const ArgumentPack&) -> ArgumentPack& = delete;
  auto operator=(ArgumentPack&&) -> ArgumentPack& = delete;

  constexpr auto get_size() const -> Count override {
    return expressions.get_size();
  }

  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;

  auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
      -> Bool override;

  auto get_fitted_at(
      const Ttx::Concept::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<
          const Ttx::Concept::Abstract&,
          Ttx::Concept::Layout::Errors> override;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Materializations& materializations,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool;

  constexpr auto is_named() const -> Bool { return named; }

  constexpr auto get_label(Count index) const -> Ttx::Lexical::Token {
    if (!named || index >= labels.get_size()) {
      return {};
    }

    return labels.get_data()[index];
  }

  constexpr auto get_label_spelling(Count index) const
      -> Perimortem::Core::View::Bytes {
    Ttx::Lexical::Token label = get_label(index);
    return label ? label.caculate_text(source)
                 : Perimortem::Core::View::Bytes();
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  ArgumentPack(
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          expressions,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> labels,
      Bool named,
      Ttx::Lexical::Anchor anchor);

  auto has_unique_labels() const -> Bool;

  Perimortem::Core::View::Bytes source;
  Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
      expressions;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> labels;
  Bool named;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language
