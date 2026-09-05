// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Language::Model {

class Pack;
class Type;

// Admission owns the language rule that accepts value flow which does not fit
// by Layout alone. Ordinary Domains need no such owner. Scalar conversions,
// optional flow, erased implementations, and byte views publish this route
// only where their concrete Library meaning can justify the wider relation.
class Admission : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("admission"_view);
  TTX_EMPTY_DOCUMENTATION();

  virtual auto accepts(const Pack& source) const -> Bool = 0;

  // Admission may construct a language-owned replacement for accepted flow.
  // An empty result keeps the original Pack, while accepts() separately states
  // whether the relationship is valid. Keeping that transformation here
  // prevents Type from becoming a universal conversion or initialization
  // service merely because it names the receiving Domain.
  virtual auto admit(Perimortem::Memory::Allocator::Arena& arena, Pack& source)
      const -> Perimortem::Core::Option<Pack&> = 0;
};

template <typename Owner>
class OwnedAdmission final : public Admission {
 public:
  explicit OwnedAdmission(const Owner& owner) : owner(owner) {}

  auto accepts(const Pack& source) const -> Bool override {
    return owner.accepts(source);
  }

  auto admit(Perimortem::Memory::Allocator::Arena& arena, Pack& source) const
      -> Perimortem::Core::Option<Pack&> override {
    if constexpr (requires { owner.create_admitted(arena, source); }) {
      return owner.create_admitted(arena, source);
    }
    return {};
  }

 private:
  const Owner& owner;
};

auto admit(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena,
    Pack& source) -> Perimortem::Core::Option<Pack&>;

}  // namespace Tetrodotoxin::Library::Language::Model
