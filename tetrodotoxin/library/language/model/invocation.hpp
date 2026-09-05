// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vector>

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/model/requirement.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Invocation is Library's operational contract for a completed Callable. One
// input Pack follows the Callable parameter Layout, including an explicit self
// position when the callable requires one. The caller supplies Context, so the
// returned Pack remains an immutable projection instead of exposing a Library
// execution object through the ABI.
class Invocation {
 public:
  static auto requirement() -> ttx_abstract;
  static auto operation() -> ttx_abstract;

  static auto call(ttx_abstract callable, ttx_pack input, ttx_context context)
      -> Ttx::PackObservation;

  // Native Library implementations may recover their own producer objects
  // after the public owner requirement succeeds. A foreign producer remains in
  // the Pack but has no local C++ Pack view, allowing the concrete operation to
  // reject or negotiate another contract without a cast across the ABI.
  static auto local_inputs(ttx_pack input)
      -> Perimortem::Core::Option<std::vector<const Pack*>>;
  static void return_pack(
      Perimortem::Core::Option<Pack&> pack,
      ttx_context context,
      ttx_pack_result result);
};

}  // namespace Tetrodotoxin::Library::Language::Model
