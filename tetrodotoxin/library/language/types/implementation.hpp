// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/model/completion.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/ffi/cpp/domain.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Implementation is the explicit Library value that erases one accepted
// Object behind a semantic requirement. It retains no member inventory or
// runtime table. The candidate owner answers satisfaction and the ABI Terminal
// derives the immutable Projection used by native code.
class Implementation : public Model::Type, public Model::Completion {
 public:

  constexpr Implementation(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Domain& requirement)
      : name(name),
        requirement(&requirement),
        admission(*this),
        initialization(*this) {}

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto accepts(const Model::Pack& source) const -> Bool;

  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  auto validate_layout(Ttx::Lexical::Cursor& cursor) const -> Bool override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_requirement() const -> const Ttx::Model::Domain& {
    return *requirement;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Domain* requirement;
  Model::OwnedAdmission<Implementation> admission;
  Model::OwnedInitialization<Implementation> initialization;
  static constexpr Ttx::Documentations::Comment documentation{
    "Carries one accepted Object with its target Projection."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
