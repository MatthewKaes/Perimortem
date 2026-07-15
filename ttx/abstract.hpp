// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/attribute.hpp"
#include "ttx/documentation.hpp"
#include "ttx/function.hpp"
#include "ttx/layout.hpp"
#include "ttx/member.hpp"

namespace Ttx {

// Abstract contains all identity information for the TTX model and represents a
// named source. Abstract does not support nullability and should only be used
// with other valid Abstracts.
//
// All other TTX components build off of Type. Additional systems and also build
// off of type.
class Abstract {
 public:
  constexpr Abstract(
      Perimortem::Core::View::Bytes name,
      Layout layout = Layout(),
      Perimortem::Core::View::Vector<Attribute> attributes =
          Perimortem::Core::View::Vector<Attribute>(),
      Documentation documentation = Documentation())
      : name(name), layout(layout), documentation(documentation) {}

  virtual auto canonicalize() const -> const Type& = 0;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  constexpr auto get_layout() const -> Layout { return layout; }

 private:
  Perimortem::Core::View::Bytes name;
  Layout layout;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Documentation documentation;
};

}  // namespace Ttx
