// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Slice is the safe indexed element or contiguous range access. Element access
// supplies one scalar value. Range access supplies a fixed size Pack whose real
// producer remains this Slice expression. It does not eagerly materialize a
// View or anonymous aggregate Type. Writable reference selection belongs to the
// separate bracket access form.
class Slice : public Expression {
 public:
  TTX_CONTRACT(Slice, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& index,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  TTX_NAME("Slice"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto fits(const Ttx::Model::Type& target) const -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  // The first operand is the scalar index or the first position of a range.
  constexpr auto get_index() const -> const Model::Pack& { return first; }

  constexpr auto get_count() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return count.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](Model::Pack* selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return *selected;
        });
  }

  constexpr auto get_element_type() const
      -> Perimortem::Core::Option<const Model::Type&> {
    return element_type.visit(
        []() -> Perimortem::Core::Option<const Model::Type&> { return {}; },
        [](const Model::Type* selected)
            -> Perimortem::Core::Option<const Model::Type&> {
          return *selected;
        });
  }

  constexpr auto get_fallback() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return fallback.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](Model::Pack* selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return *selected;
        });
  }

  constexpr auto get_range_count() const -> Perimortem::Core::Option<Count> {
    return range_count;
  }

 private:
  auto evaluate_fold() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Expression::Error>;
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& index,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Model::Pack& receiver;
  Model::Pack& first;
  Perimortem::Core::Option<Model::Pack*> count;
  Perimortem::Core::Option<const Model::Type*> element_type;
  Perimortem::Core::Option<Model::Pack*> fallback;
  Perimortem::Core::Option<Count> range_count;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> range_layout;
};

}  // namespace Tetrodotoxin::Library::Language::Access
