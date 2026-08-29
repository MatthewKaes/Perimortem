// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Propagate delegates its continuation and escape edges to the exact receiver
// Type. Option and inactive Flags escape with empty flow, while Result supplies
// one typed error Pack that the enclosing Function must receive explicitly.
class Propagate : public Expression {
 public:
  TTX_CONTRACT(Propagate, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Ttx::Lexical::Anchor anchor) -> Propagate&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Propagate"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  constexpr auto get_escape() const -> const Model::Pack& {
    return escape.get();
  }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  class ErrorEscape : public Expression {
   public:
    TTX_CONTRACT(ErrorEscape, Expression);
    TTX_NAME("Propagation error"_view);
    TTX_EMPTY_DOCUMENTATION();

    constexpr auto get_type() const -> const Model::Type& override {
      return type;
    }

   private:
    friend class Propagate;

    constexpr explicit ErrorEscape(const Model::Type& type)
        : Expression({}), type(type) {}

    const Model::Type& type;
  };

  constexpr Propagate(
      Model::Pack& receiver,
      Model::Pack& empty_escape,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), escape(empty_escape) {}

  Model::Pack& receiver;
  Ttx::Model::PackReference<Model::Pack> escape;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      receiver_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      continuation_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      error_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
