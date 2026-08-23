// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Expressions {

// Initializer owns one explicit construction expression and its evaluation
// order. An omitted argument list requests the exact Library Type default. A
// supplied Pack is handed to that same Type so the expression never inspects a
// concrete construction category. Synthetic aggregate defaults retain their
// exact Type without inventing a source Anchor.
class Initializer : public Expression {
 public:
  TTX_CONTRACT(Initializer, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      TypeReference target_reference,
      Model::Pack& arguments,
      Ttx::Lexical::Anchor anchor) -> Initializer&;

  // Synthetic aggregate defaults retain their exact target Type and one real
  // child Pack per completed element or state Field.
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Pack>>
          values) -> Initializer&;

  // A restored Interface aggregate delegates construction to its provider's
  // native Type operation. The Initializer remains the produced Pack identity.
  // no semantic Callable or copied Field model is introduced.
  static auto create_provider(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Model::Pack& arguments) -> Initializer&;

  Initializer(const Initializer&) = delete;
  Initializer(Initializer&&) = delete;
  auto operator=(const Initializer&) -> Initializer& = delete;
  auto operator=(Initializer&&) -> Initializer& = delete;

  TTX_NAME("Initializer"_view);

  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto fits(const Ttx::Model::Type& target) const -> Bool override;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  auto get_completed_values() const
      -> Perimortem::Core::Option<const Model::Pack&>;

  constexpr auto get_arguments() const -> const Model::Pack& {
    return arguments;
  }

  constexpr auto uses_provider() const -> Bool { return provider; }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  Initializer(
      Perimortem::Core::Option<TypeReference> target_reference,
      Model::Pack& arguments,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  Perimortem::Core::Option<TypeReference> target_reference;
  Model::Pack& arguments;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      expected_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>>
      completed_values;
  Bool provider = False;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
