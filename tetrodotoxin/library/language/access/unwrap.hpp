// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Unwrap observes one Option and always produces its exact element Type. A
// present value supplies the retained payload while absence creates a fresh
// semantic default owned by that element Type.
class Unwrap : public Expression {
 public:
  TTX_CONTRACT(Unwrap, Expression);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME("Unwrap"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto finalize() -> void override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  constexpr Unwrap(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), domain(domain), receiver(receiver) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Expression& receiver;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      element_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
