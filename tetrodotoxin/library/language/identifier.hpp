// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Identifier is one authored root name Expression. It retains the exact Token
// and an Arena-stable spelling, then delays binding until the Type-defining
// pass is complete. A Type result is a Descriptor value; an Addressable result
// keeps its ordinary value Type. Both remain opaque until link selects them.
class Identifier : public Expression {
 public:
  TTX_CONTRACT(Identifier, Expression, 0xd747288c3703480b, 0x9cd07fbc8a1a7a84);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes source,
      Ttx::Lexical::Anchor anchor) -> Identifier& {
    Perimortem::Core::View::Bytes name =
        domain.proxy(token.caculate_text(source));
    return Expression::create_authored<Identifier>(
        domain, anchor, [&](auto authored) -> Identifier {
          return Identifier(token, name, authored);
        });
  }

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Materializations& materializations,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto get_result() const -> const Ttx::Concept::Abstract& override;

  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Identifier(
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), token(token), name(name) {}

  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      result;
};

}  // namespace Tetrodotoxin::Library::Language
