// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Expressions {

// Initializer owns one aggregate initialization operation. Authored `new[Type]`
// carries its exact Object Type while the receiving declaration owns
// publication. Synthetic defaults retain an exact aggregate Type without
// inventing a source Anchor.
class Initializer : public Expression {
 public:
  TTX_CONTRACT(Initializer, Expression, 0x921ccce2e4934c8d, 0x9d19d3434e18f49a);

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Initializer&>;

  // Synthetic aggregate defaults retain their exact target Type and one real
  // child Pack per completed element or state Field.
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Pack>>
          values) -> Initializer&;

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
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  auto finalize() -> void override;

  auto get_completed_values() const
      -> Perimortem::Core::Option<const Model::Pack&>;

 private:
  Initializer(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<TypeReference> target_reference,
      Model::Pack& arguments,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  auto supplies(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      const Field& field) const -> Bool;

  auto select_supplied(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      const Field& field) const -> Perimortem::Core::Option<const Model::Pack&>;

  auto has_mandatory_cycle(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Types::Object>> path) const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::Option<TypeReference> target_reference;
  Model::Pack& arguments;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      expected_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>>
      completed_values;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
