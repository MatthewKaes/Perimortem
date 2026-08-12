// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Pack is the Library lifecycle shared by every produced value flow. A scalar
// Expression and an authored parenthesized group expose the same link, fitting,
// and finalization surface, so consumers never branch on their concrete carrier
// merely to obtain the complete TTX Layout.
//
// The TTX Pack remains the host-neutral semantic contract. This derived owner
// adds only the Library stages needed to bind authored Expressions. A Pack may
// be observed through get_layout() only after resolve() returns this exact
// Pack; empty output is then real zero-value flow rather than an incomplete
// sentinel.
class Pack : public Ttx::Model::Pack {
 public:
  TTX_CONTRACT(Pack, Ttx::Model::Pack, 0xa9a301f166674390, 0x990ea203734fbd91);

  virtual auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool = 0;

  // The scalar output query is a convenience over the Pack's completed Layout.
  // It is Invalid for empty or multi-value flow and never materializes an
  // aggregate Type merely to make the query succeed.
  virtual auto get_type() const -> const Ttx::Concept::Abstract&;

  // Pack fitting preserves each real producer's value rules before falling
  // back to its identity-free output Layout. This is what lets a Constant own
  // contextual scalar conversion while grouped and named flows retain their
  // concrete Layout ordering and names.
  auto fits(const Ttx::Concept::Layout& target) const -> Bool override;
  auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
      -> Bool;
  auto fits_entry(
      const Ttx::Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool;

  auto get_fitted_at(
      const Ttx::Concept::Layout& target,
      Count target_offset,
      Count target_index) const -> Perimortem::Utility::
      Result<const Ttx::Concept::Abstract&, Ttx::Concept::Layout::Errors>;

  // A general Pack fits a Type through both complete Layouts. Scalar contextual
  // conversions belong to Expression and Constant; applying them here would
  // let one-entry grouped flow ignore the rest of a structural Type Layout.
  virtual auto fits(const Ttx::Model::Type& target) const -> Bool;

  // Finalization visits the real child Packs in evaluation order. It does not
  // imply that empty or multi-value flow can be constant-folded into one value.
  virtual auto finalize() -> void = 0;

  static auto create_empty(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor = {}) -> Pack&;

  // A concrete producer may compose already retained child Packs when its
  // semantic result is genuinely grouped flow. Positional composition flattens
  // child Layouts; named composition requires one produced value per name. The
  // children remain the only value identities and evaluation edges.
  static auto create_group(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Pack>> entries,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names = {},
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor = {}) -> Pack&;

 protected:
  constexpr Pack() = default;
};

}  // namespace Tetrodotoxin::Library::Language::Model
