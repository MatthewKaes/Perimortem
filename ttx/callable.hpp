// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abstract.hpp"

namespace Ttx {

// Callable exposes one candidate beside its current parameter and result
// Layouts. The view records call shape only. Selection, invocation, execution,
// failure, and target calling convention remain with concrete concepts and
// Terminals.
class Callable : public Abstract {
 public:
  Callable();
  Callable(uint64_t authority, uint64_t value);

  virtual auto parameters() const -> ttx_layout = 0;
  virtual auto results() const -> ttx_layout = 0;
  void callable(ttx_abstract self, ttx_callable_result result) const final;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation final;

 private:
  struct Binding {
    ttx_callable_ops operations;
    const Callable* owner;
  };

  static auto select(ttx_callable self) -> const Callable&;
  static auto TTX_CALL candidate(ttx_callable self) -> ttx_abstract;
  static auto TTX_CALL get_parameters(ttx_callable self) -> ttx_layout;
  static auto TTX_CALL get_results(ttx_callable self) -> ttx_layout;

  const Binding binding;
};

}  // namespace Ttx
