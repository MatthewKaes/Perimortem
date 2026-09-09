// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Unwrap is a special control flow access (post fix !) that will default a
// value in the middle of an access expression if an Option failed to return
// one allowing it to be chained.
//
// It can be used for scenarios where a default value is still acceptable and
// doesn't act as an outright failure.
class Unwrap : public Expression {
 public:
  TTX_CONTRACT(Unwrap, Expression);

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

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  constexpr auto get_fallback() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return fallback.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](const Ttx::Model::PackReference<Model::Pack>& selected)
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
      Model::Pack& receiver,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver) {}

  Model::Pack& receiver;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Ttx::Model::PackReference<Model::Pack>> fallback;
};

}  // namespace Tetrodotoxin::Library::Language::Access
