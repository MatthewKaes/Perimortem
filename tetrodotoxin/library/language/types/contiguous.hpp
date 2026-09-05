// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/iteration.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Contiguous is the shared Type contract for indexed value storage. The
// receiver remains one value of its exact concrete Type while this category
// exposes the exact element Type consumed by Slice. Access retains the writable
// subset used by Index.
class Contiguous : public Model::Type {
 public:

  Contiguous() : iteration(*this), initialization(*this) {}

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  virtual constexpr auto get_element_type() const -> const Model::Type& = 0;

  virtual auto initialize_default(Perimortem::Memory::Allocator::Arena& arena)
      const -> Perimortem::Core::Option<Model::Pack&> = 0;

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return get_element_type().get_declaration_anchor();
  }

  auto accepts_binding(const Ttx::Concept::Layout& bindings) const -> Bool;

 private:
  class Iteration final : public Model::Iteration {
   public:
    explicit Iteration(const Contiguous& owner) : owner(owner) {}

    auto accepts_binding(const Ttx::Concept::Layout& bindings) const
        -> Bool override {
      return owner.accepts_binding(bindings);
    }

   private:
    const Contiguous& owner;
  };

  Iteration iteration;
  Model::OwnedInitialization<Contiguous> initialization;
};

}  // namespace Tetrodotoxin::Library::Language::Types
