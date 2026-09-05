// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Value is the Library scalar representation protocol. Width is semantic bit
// precision, while size and alignment describe target storage. Keeping them
// distinct prevents category eligibility from implying promotion or packing.
class Value : public Model::Type {
 public:

  Value() : admission(*this), initialization(*this) {}

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  // Folding may retain the selected Type while producing an incompatible
  // Constant carrier. The scalar domain owns that proof so every consumer can
  // reject malformed folded values without enumerating Constant subclasses.
  virtual auto accepts_constant(const Ttx::Concept::Abstract&) const
      -> Bool = 0;

  // Builtin scalars carry the same Library meaning in every source Monograph.
  // Cross source flow therefore admits the matching category and precision
  // without treating an arbitrary authored Type or physical representation as
  // interchangeable.
  auto accepts(const Model::Pack& source) const -> Bool;

  // Builtin scalar identities are Monograph local, while their category and
  // precision are language wide meaning. This relation lets composed Dialects
  // preserve one operand identity without treating authored Types or target
  // storage as interchangeable.
  auto is_equivalent(const Value& source) const -> Bool;

  virtual auto initialize_default(Perimortem::Memory::Allocator::Arena& arena)
      const -> Perimortem::Core::Option<Model::Pack&> = 0;

  auto initialize_supplied(
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto initialize_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Model::Pack&>;

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;

 private:
  Model::OwnedAdmission<Value> admission;
  Model::OwnedInitialization<Value> initialization;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
