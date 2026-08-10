// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Access is one writable contiguous storage Type. It retains the element edge
// and original Generic argument so consumers query the same real identity
// without a registry or copied shape.
class Access : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(
      Access,
      Ttx::Model::Type,
      0x9297f2706d2e464e,
      0x82d4b4fece93716d);

  constexpr Access(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), argument(element) {}

  TTX_NAME(name);

  TTX_CONSTEXPR_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_arguments() const
      -> Perimortem::Core::View::Vector<Generic::Argument> {
    return {&argument, 1};
  }

  constexpr auto get_element_type() const -> const Ttx::Model::Type& {
    return *argument.find<const Ttx::Model::Type&>();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Generic::Argument argument;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides writable access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
