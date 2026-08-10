// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Identifier is one authored use of an Addressable route. Every use remains a
// distinct Expression even when several uses link to the same exact
// Addressable. A missing route keeps its authored identity and may link after
// the surrounding graph has completed.
class Identifier : public Expression {
 public:
  TTX_CONTRACT(Identifier, Expression, 0xd747288c3703480b, 0x9cd07fbc8a1a7a84);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor) -> Identifier& {
    return Expression::create_authored<Identifier>(
        domain, anchor,
        [&](auto source) -> Identifier { return Identifier(route, source); });
  }

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& context,
      Materializations& materializations) -> Bool override;

  TTX_NAME(route);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  auto get_addressable() const
      -> Perimortem::Core::Option<const Ttx::Model::Addressable&>;

 private:
  constexpr Identifier(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), route(route) {}

  Perimortem::Core::View::Bytes route;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      addressable;
};

}  // namespace Tetrodotoxin::Library::Language
