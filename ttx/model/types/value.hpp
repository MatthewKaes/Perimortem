// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

namespace Ttx::Model::Types {

// Value Types are terminal bit interpretations with no structural
// representation. Type supplies their one entry Value Layout, so the exact
// scalar identity closes recursive semantic shape instead of laundering the
// scalar through an empty Layout.
//
// Any further contextual resolution on the type terminates to `Invalid`.
class Value : public Type {
 public:
  TTX_CONTRACT(Value, Type, 0x0904f828ec2a489f, 0x978eb1f113cb771f);

  TTX_INVALID_CONTEXT;

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;
};

}  // namespace Ttx::Model::Types
