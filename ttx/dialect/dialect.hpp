// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Tetrodotoxin::Dialect {

class Dialect {
 public:
  virtual auto get_name() const -> Perimortem::Core::View::Bytes = 0;
  virtual auto parse(Context ctx) const;

  static auto add(const Dialect& dialect) -> void;
  static auto find(Perimortem::Core::View::Bytes name) -> const Dialect&;
  static auto get_dialects() -> Perimortem::Core::View::Vector<const Dialect*>;
};

}  // namespace Tetrodotoxin::Dialects
