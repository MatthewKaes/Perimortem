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
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Unwrap"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  auto lower(Llvm::Builder& body) const -> Bool override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_fallback() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return fallback.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](const Ttx::Concept::Reference<Model::Pack>& selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return selected.get();
        });
  }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  constexpr Unwrap(
      Expression& receiver,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver) {}

  Expression& receiver;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>> fallback;
};

}  // namespace Tetrodotoxin::Library::Language::Access
