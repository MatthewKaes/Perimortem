// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Range is one lazy integer sequence Type. It retains the exact element edge
// and original Generic argument without claiming contiguous storage or state.
class Range : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(Range, Ttx::Model::Type, 0x824db901761e4b83, 0xa54b4be20f4144b7);

  constexpr Range(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), argument(element) {}

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
    "Provides a lazy ascending integer sequence."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
