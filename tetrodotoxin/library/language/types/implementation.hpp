// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/bootstrap/model/documentations/comment.hpp"
#include "ttx/bootstrap/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Implementation is the explicit Library value that erases one accepted
// Object behind a semantic requirement. It retains no member inventory or
// runtime table. The candidate owner answers satisfaction and the ABI Terminal
// derives the immutable Projection used by native code.
class Implementation : public Model::Type {
 public:
  TTX_CONTRACT(Implementation, Model::Type);

  constexpr Implementation(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& requirement)
      : name(name), requirement(&requirement) {}

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts(const Model::Pack& source) const -> Bool override;

  auto validate_layout(Ttx::Lexical::Cursor& cursor) const -> Bool override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_requirement() const -> const Ttx::Model::Type& {
    return *requirement;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Type* requirement;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Carries one accepted Object with its target Projection."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
