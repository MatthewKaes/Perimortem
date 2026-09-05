// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/iteration.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Range is one lazy integer sequence Type. It retains the exact element edge
// without claiming contiguous storage, state, or ownership of its Generic key.
class Range : public Model::Type {
 public:

  constexpr Range(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element)
      : name(name), element(element), iteration(*this), initialization(*this) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto accepts_binding(const Ttx::Concept::Layout& bindings) const -> Bool;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return element.get_declaration_anchor();
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  class Iteration final : public Model::Iteration {
   public:
    explicit Iteration(const Range& owner) : owner(owner) {}

    auto accepts_binding(const Ttx::Concept::Layout& bindings) const
        -> Bool override {
      return owner.accepts_binding(bindings);
    }

   private:
    const Range& owner;
  };
  Iteration iteration;
  Model::OwnedInitialization<Range> initialization;
  static constexpr Ttx::Documentations::Comment documentation{
    "Provides a lazy ascending integer sequence."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
