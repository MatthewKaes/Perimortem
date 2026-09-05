// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Builtin::Enum {

// Size is the Static immutable case count of one Enumeration.
class Size : public Language::Model::Addressable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "size"_view;


  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Types::Unsigned& type,
      Count count) -> Size&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_type() const
      -> const Language::Model::Types::Unsigned& override {
    return type;
  }

  constexpr auto resolve() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto resolve_concept(Perimortem::Core::View::Bytes query) const
      -> const Ttx::Concept::Abstract& override {
    return query == "fold"_view
               ? *constant.get_identity()
               : Language::Model::Addressable::resolve_concept(query);
  }

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override {
    Language::Model::Addressable::visit_concepts(visitor);
    visit_concept(visitor, "fold"_view, resolve_concept("fold"_view));
  }

 private:
  constexpr Size(
      const Language::Model::Types::Unsigned& type,
      Language::Model::Pack& constant)
      : type(type), constant(constant) {}

  const Language::Model::Types::Unsigned& type;
  Language::Model::Pack& constant;

  static constexpr Ttx::Documentations::Comment documentation{
    "Provides the compile time number of cases in this Enumeration."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Enum
