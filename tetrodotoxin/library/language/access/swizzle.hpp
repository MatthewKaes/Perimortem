// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Swizzle selects and reorders named values from one receiver Pack. A named
// Pack contributes the real producer retained at each selected source index;
// one multi-result producer may therefore supply several distinct slots. A
// scalar Expression may instead contribute the named Addressables of its
// output Type, in which case each selected slot is a real Address Expression
// bound to that receiver. The result is positional Pack flow and never an
// eagerly materialized Type.
class Swizzle : public Expression {
 public:
  TTX_CONTRACT(Swizzle, Expression, 0xc43faea8e4984ac5, 0x81abdbb11a727513);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver,
      Ttx::Lexical::Span receiver_span)
      -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME("Swizzle"_view);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto finalize() -> void override;

  constexpr auto get_receiver() const -> const Language::Model::Pack& {
    return receiver;
  }

 private:
  Swizzle(
      Perimortem::Memory::Allocator::Arena& domain,
      Language::Model::Pack& receiver,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        domain(domain),
        receiver(receiver),
        name_tokens(name_tokens),
        names(names),
        selections(domain),
        projections(domain) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Language::Model::Pack& receiver;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<Count> selections;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      projections;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> output;
};

}  // namespace Tetrodotoxin::Library::Language::Access
