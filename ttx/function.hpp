// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/layout.hpp"

namespace Ttx {

// Function is callable behavior attached to a type.
//
// Calls require a type because pure layouts have no function table to dispatch
// through. `Sprite->draw(...)` can ask the Sprite type for a function.
// `(.x = 2, .y = 3)->format()` is invalid because the layout has no identity
// and therefore no owner for `format`.
//
// Parameters and results are layouts. Calls, construction, returns, and shader
// boundary checks can therefore use the same Layout fit rules instead of
// growing a parallel argument model.
class Function {
 public:
  constexpr Function() = default;
  constexpr Function(
      Perimortem::Core::View::Bytes name,
      Layout parameters,
      Layout result,
      Documentation documentation = Documentation())
      : name(name),
        parameters(parameters),
        result(result),
        documentation(documentation) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_parameters() const -> Layout { return parameters; }
  constexpr auto get_result() const -> Layout { return result; }
  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  constexpr auto is_empty() const -> Bool {
    return name.is_empty() && parameters.is_empty() && result.is_empty();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Layout parameters;
  Layout result;
  Documentation documentation;
};

}  // namespace Ttx
