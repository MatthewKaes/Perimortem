// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "ttx/function.hpp"

namespace Tetrodotoxin::Abi {

// Export joins one public TTX callable path to the stable machine symbol that
// host languages use and the internal symbol that implements it.
//
// The path includes the Type or Addressable table selected by the owner that
// published the function. Keeping that decision in the path means Export does
// not cache a second surface flag or search backward from Function to Type. A
// backend can publish the export as another symbol for the same code range,
// while a generated binding presents the authored path and keeps an
// Addressable function's `self` argument explicit.
class Export {
 public:
  constexpr Export(const Export&) = default;

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes public_path,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes target_symbol,
      Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Type> type_identities)
      -> const Export*;
  static auto project(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes public_path,
      const Export& source) -> const Export*;

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return public_path;
  }

  constexpr auto get_function() const -> const Ttx::Function& {
    return function;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

  constexpr auto get_target_symbol() const -> Perimortem::Core::View::Bytes {
    return target_symbol;
  }

 private:
  Export(
      Perimortem::Core::View::Bytes public_path,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes symbol,
      Perimortem::Core::View::Bytes target_symbol)
      : public_path(public_path),
        function(function),
        symbol(symbol),
        target_symbol(target_symbol) {}

  Perimortem::Core::View::Bytes public_path;
  const Ttx::Function& function;
  Perimortem::Core::View::Bytes symbol;
  Perimortem::Core::View::Bytes target_symbol;
};

}  // namespace Tetrodotoxin::Abi
