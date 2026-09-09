// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Ttx::Concept {

// The dispatch boundary receives a runtime contract identity, so its return
// type cannot name the selected operation table at compile time. Binding
// carries the successful state and table through that boundary. The caller
// then recovers the typed query view for the contract it requested.
//
// Negotiation returns a Result containing either this pair or a Failure. An
// unsupported question may pass to the next policy, while pending and rejected
// answers stop there. Those outcomes belong to the Result rather than to a
// partially populated Binding.
class Binding {
 public:
  enum class Failure : U8 {
    Unsupported,
    Pending,
    Rejected,
  };

  template <typename Contract>
  static constexpr auto provide(const void* source,
                                const typename Contract::Operations& operations)
      -> Binding {
    return Binding(source, &operations);
  }

  // The dispatch owner must have matched Contract's identity before supplying
  // the table. Supplying a different table is undefined behavior, just as an
  // incompatible function pointer in an operation table would be.
  template <typename Contract>
  constexpr auto get() const -> typename Contract::Handle {
    return typename Contract::Handle(
        source, *static_cast<const typename Contract::Operations*>(operations));
  }

 private:
  constexpr Binding(const void* source, const void* operations)
      : source(source), operations(operations) {}

  const void* source;
  const void* operations;
};

}  // namespace Ttx::Concept
