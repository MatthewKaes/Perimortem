// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

namespace Ttx::Model::Types {

// Terminal is the Type contract for a value represented directly by a target.
// Its value width, byte size, and alignment are real facts, while register
// carriers, instructions, and calling conventions remain compiler decisions.
// Terminal Types are Layout leaves and can still resolve to their full Type
// contract.
//
// Each concrete Terminal Type publishes its fixed name, documentation, value
// width in bits, storage size in bytes, and alignment in bytes directly. The
// active toolchain constructs only the stable Types it supports and installs
// them in its own contexts. Core TTX owns no prelude or global terminal
// catalogue. A wider register or instruction does not change get_width().
class Terminal : public Type {
 public:
  using ContractOwner = Terminal;
  static constexpr Perimortem::System::Uuid contract_id{
    0x0904f828ec2a489f,
    0x978eb1f113cb771f,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Type::implements(requested);
  }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;
};

}  // namespace Ttx::Model::Types
