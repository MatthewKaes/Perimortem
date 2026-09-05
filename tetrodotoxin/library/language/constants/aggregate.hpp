// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Aggregate preserves complete empty or multi-value folding without inventing
// a value type for the group. Its Layout and child Pack view expose the same
// Constants, allowing a Terminal to reuse their representations in the order
// the folding owner established.
class Aggregate final : public Ttx::Concept::Constant, public Model::Pack {
 public:

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Model::Pack*> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names = {})
      -> Perimortem::Core::Option<Aggregate&>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto get_identity() const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_entries() const
      -> Perimortem::Core::View::Vector<Model::Pack*> override {
    return values;
  }
  constexpr auto is_complete() const -> Bool override { return True; }
  auto fits(const Ttx::Concept::Layout& target) const -> Bool override;
  auto fits(const Ttx::Model::Domain& target) const -> Bool override;
  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

 private:
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr explicit Layout(const Aggregate& aggregate)
        : aggregate(aggregate) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;
    auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;
    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const -> Perimortem::Utility::
        Result<const Ttx::Concept::Abstract&, Errors> override;

   private:
    const Aggregate& aggregate;
  };

  Aggregate(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Model::Pack*> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names);

  Perimortem::Memory::Managed::Vector<Model::Pack*> values;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Bytes name;
  Layout layout;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
