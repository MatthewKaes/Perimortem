// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/layout.hpp"

namespace Tetrodotoxin::Diagnostics {

// Stateless spelling suggestions for failed semantic name lookups.
//
// Exact lookup remains with the semantic owner. These operations borrow the
// complete candidate set for one selection transaction and return a formatted
// hint without retaining candidate or result state. Candidate order is
// preference order: the first exact or one-edit name returns immediately,
// while the first two-edit name is only a fallback.
class Suggestions {
 public:
  static auto possible_candidate(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> candidates)
      -> Perimortem::Core::View::Bytes;
  static auto possible_candidate(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Ttx::Layout candidates) -> Perimortem::Core::View::Bytes;

 private:
  using CandidateName = Perimortem::Core::View::Bytes (*)(const void*, Count);

  static auto possible_index(
      Perimortem::Core::View::Bytes name,
      const void* candidates,
      Count candidate_count,
      CandidateName candidate_name) -> Count;
  static auto view_name(const void* candidates, Count index)
      -> Perimortem::Core::View::Bytes;
  static auto member_name(const void* candidates, Count index)
      -> Perimortem::Core::View::Bytes;
  static auto format(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes candidate) -> Perimortem::Core::View::Bytes;
  static auto distance(
      Perimortem::Core::View::Bytes left,
      Perimortem::Core::View::Bytes right) -> Count;
};

}  // namespace Tetrodotoxin::Diagnostics
