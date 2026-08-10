// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constants/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// True is the positive Flag Constant refinement. The retained Flag Type still
// owns representation while this identity exposes the closed logical value.
class True : public Flag {
 public:
  TTX_CONTRACT(True, Flag, 0x622f43659a9a4cc5, 0x935e114a9c2fc915);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Model::Types::Flag& type,
      Ttx::Lexical::Anchor anchor) -> True& {
    return Expression::create_authored<True>(
        domain, anchor,
        [&](auto source) -> True { return True(type, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Model::Types::Flag& type) -> True& {
    return Expression::create_synthetic<True>(
        domain, [&](auto source) -> True { return True(type, source); });
  }

 private:
  constexpr True(
      const Ttx::Model::Types::Flag& type,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Flag(type, ::True, anchor) {}
};

}  // namespace Tetrodotoxin::Library::Language::Constants
