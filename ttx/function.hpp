// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/layout.hpp"

namespace Ttx {

// Function is a callable name with parameter and result layouts.
//
// It deliberately does not classify itself as Type or Addressable behavior.
// That fact belongs to the Ttx::Type table containing the function. Keeping
// ownership there prevents parameter names such as `self` from becoming a
// hidden dispatch flag and lets dialects choose their own receiver shorthand.
//
// Parameters and results are layouts. Calls, construction, returns, and shader
// boundary checks can therefore use the same Layout fit rules instead of
// growing a parallel argument model. An Addressable call contributes its
// receiver as the first argument before fitting the complete parameter layout.
// Consumers never manufacture a second call-site layout with that entry
// removed.
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
