// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// View is one read only contiguous storage Type. It retains the element edge
// and original Generic argument so consumers query the same real identity
// without a registry or copied shape.
class View : public Ttx::Model::Type {
 public:
  using ClassCatagory = View;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdbd463c86a2c4a08,
    0xb42869ffad9e4fc9,
  };

  constexpr View(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), argument(element) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Ttx::Model::Type::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

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
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
