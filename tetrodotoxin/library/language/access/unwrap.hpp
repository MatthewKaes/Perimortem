// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Unwrap observes one Option and always produces its exact element Type. A
// present value supplies the retained payload while absence creates a fresh
// semantic default owned by that element Type.
class Unwrap : public Expression {
 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Ttx::Lexical::Anchor anchor) -> Unwrap&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Unwrap"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  constexpr auto get_fallback() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return fallback.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](Model::Pack* selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return *selected;
        });
  }

 private:
  auto evaluate_fold() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Expression::Error>;
  constexpr Unwrap(
      Model::Pack& receiver,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver) {}

  Model::Pack& receiver;
  Perimortem::Core::Option<const Model::Type*> element_type;
  Perimortem::Core::Option<Model::Pack*> fallback;
};

}  // namespace Tetrodotoxin::Library::Language::Access
