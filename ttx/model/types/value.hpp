// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

namespace Ttx::Model::Types {

// Value types that do not have a composite representation and represent some
// kind of terminal bit interpretation (memory or storage).
//
// Any further contextual resolution on the type terminates to `Invalid`.
class Value : public Type {
 public:
  TTX_CONTRACT(Value, Type, 0x0904f828ec2a489f, 0x978eb1f113cb771f);

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;
};

}  // namespace Ttx::Model::Types
