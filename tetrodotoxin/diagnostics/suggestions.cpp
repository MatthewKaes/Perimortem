// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/diagnostics/suggestions.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

auto Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
    Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<View::Bytes> candidates) -> View::Bytes {
  Count index = possible_index(
      name, candidates.get_data(), candidates.get_size(), view_name);
  return index == Count(-1) ? View::Bytes() : format(arena, candidates[index]);
}

auto Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
    Allocator::Arena& arena,
    View::Bytes name,
    Ttx::Layout candidates) -> View::Bytes {
  View::Vector<Ttx::Member> members = candidates.get_members();
  Count index =
      possible_index(name, members.get_data(), members.get_size(), member_name);
  return index == Count(-1) ? View::Bytes()
                            : format(arena, members[index].get_name());
}

auto Tetrodotoxin::Diagnostics::Suggestions::possible_index(
    View::Bytes name,
    const void* candidates,
    Count candidate_count,
    CandidateName candidate_name) -> Count {
  Count best_index = Count(-1);
  for (Count i = 0; i < candidate_count; i++) {
    View::Bytes candidate = candidate_name(candidates, i);
    if (candidate.is_empty()) {
      continue;
    }

    // Check if the candidate is good enough that we should just use it.
    Count candidate_distance = distance(name, candidate);
    if (candidate_distance <= 1) {
      return i;
    }

    // If this is the best candidate we've found so far then use it.
    if (candidate_distance == 2 && best_index == Count(-1)) {
      best_index = i;
    }
  }

  return best_index;
}

auto Tetrodotoxin::Diagnostics::Suggestions::view_name(
    const void* candidates,
    Count index) -> View::Bytes {
  return static_cast<const View::Bytes*>(candidates)[index];
}

auto Tetrodotoxin::Diagnostics::Suggestions::member_name(
    const void* candidates,
    Count index) -> View::Bytes {
  return static_cast<const Ttx::Member*>(candidates)[index].get_name();
}

auto Tetrodotoxin::Diagnostics::Suggestions::format(
    Allocator::Arena& arena,
    View::Bytes candidate) -> View::Bytes {
  Managed::Bytes hint(arena);
  hint.concat("Did you mean `"_view);
  hint.concat(candidate);
  hint.concat("`?"_view);
  return hint;
}

auto Tetrodotoxin::Diagnostics::Suggestions::distance(
    View::Bytes left,
    View::Bytes right) -> Count {
  Count maximum_distance =
      Math::max(left.get_size(), right.get_size()) <= 4 ? Count(1) : Count(2);

  // Use the shorter name for rows so work scales with the smaller input. The
  // final cell remains within the band because names farther apart in length
  // than the accepted distance cannot be suggestions.
  if (left.get_size() > right.get_size()) {
    Data::swap(left, right);
  }

  if (right.get_size() - left.get_size() > maximum_distance) {
    return Count(-1);
  }

  // At most five cells are live in either row for the diagnostic limit of two
  // edits. Eight slots let absolute columns wrap with a binary mask without
  // any live cells colliding, regardless of the input lengths.
  constexpr Count row_size = 8;
  constexpr Count row_mask = row_size - 1;
  Bits_8 rows[2][row_size];
  Count previous_row = 0;
  Count current_row = 1;
  Bits_8 unreachable = Bits_8(maximum_distance + 1);
  for (Count row = 0; row < 2; row++) {
    for (Count slot = 0; slot < row_size; slot++) {
      rows[row][slot] = unreachable;
    }
  }

  Count initial_end = Math::min(right.get_size(), maximum_distance);
  for (Count column = 0; column <= initial_end; column++) {
    rows[previous_row][column & row_mask] = Bits_8(column);
  }

  for (Count i = 1; i <= left.get_size(); i++) {
    // This row previously held older distances. Resetting all eight slots
    // prevents wrapped columns outside the active band from looking reachable.
    for (Count slot = 0; slot < row_size; slot++) {
      rows[current_row][slot] = unreachable;
    }

    Count first_column = i > maximum_distance ? i - maximum_distance : 0;
    Count last_column = Math::min(right.get_size(), i + maximum_distance);
    Bits_8 row_minimum = unreachable;
    if (first_column == 0) {
      rows[current_row][0] = Bits_8(i);
      row_minimum = rows[current_row][0];
    }

    Count column = Math::max(Count(1), first_column);
    for (; column <= last_column; column++) {
      Bits_8 deletion = Bits_8(rows[previous_row][column & row_mask] + 1);
      Bits_8 insertion = Bits_8(rows[current_row][(column - 1) & row_mask] + 1);
      Bits_8 substitution = Bits_8(
          rows[previous_row][(column - 1) & row_mask] +
          (left[i - 1] == right[column - 1] ? 0 : 1));
      Bits_8 candidate_distance = Math::min(
          unreachable, Math::min(Math::min(deletion, insertion), substitution));
      rows[current_row][column & row_mask] = candidate_distance;
      row_minimum = Math::min(row_minimum, candidate_distance);
    }

    // Once every reachable prefix exceeds the limit, later rows cannot return
    // to an accepted edit path.
    if (row_minimum > maximum_distance) {
      return Count(-1);
    }

    Data::swap(previous_row, current_row);
  }

  Count result = rows[previous_row][right.get_size() & row_mask];
  return result <= maximum_distance ? result : Count(-1);
}
