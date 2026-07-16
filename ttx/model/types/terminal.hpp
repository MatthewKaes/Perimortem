// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/type.hpp"

namespace Ttx::Model::Types {

// Terminal is the Type contract for a value represented directly by a target.
// Its byte size and alignment are real storage facts, while register carriers,
// instructions, and calling conventions remain compiler decisions. Terminal
// Types are Layout leaves and can still resolve to their full Type contract.
//
// The implementing toolchain owns the name, supported widths, and validity of
// the size/alignment pair. Core TTX owns no concrete Terminal record or global
// terminal catalogue. The set of TTX provided terminals are strictly optional
// and can be extended as appropriate for a given architecture.
class Terminal : public Type {
 public:
  using ContractOwner = Terminal;
  static constexpr Perimortem::System::Uuid contract_id{
    0x0904f828ec2a489f,
    0x978eb1f113cb771f,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Type::implements(requested);
  }

  auto get_layout() const -> const Layouts::Structured& final { return layout; }

  virtual auto get_size() const -> Count = 0;
  virtual auto get_alignment() const -> Count = 0;

 private:
  inline static const Layouts::Structured layout;
};

}  // namespace Ttx::Model::Types
