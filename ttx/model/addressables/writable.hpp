// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/addressable.hpp"

namespace Ttx::Model::Addressables {

// Writable proves that assignment may target one Addressable. Assignment
// legality depends on this contract rather than a mutability Kind stored on
// every address. The active Dialect still owns how a successful write is
// evaluated or lowered.
//
// get_read_only() returns the durable Addressable edge used when owner-written
// state is exposed without granting consumers write capability. The returned
// object has the same name, documentation, and resolved Type, does not prove
// Writable, and remains stable for the lifetime of this object. It is a real
// graph edge supplied by the concrete producer, not an Alias that resolves back
// to the Writable identity and restores the hidden capability.
class Writable : public Addressable {
 public:
  using ContractOwner = Writable;
  static constexpr Perimortem::System::Uuid contract_id{
    0x259bbcf95820479b,
    0xa990951818897fa1,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Addressable::implements(requested);
  }

  virtual constexpr auto get_read_only() const -> const Addressable& = 0;
};

}  // namespace Ttx::Model::Addressables
