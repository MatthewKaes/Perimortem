// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/isa/base/definition.hpp"
#include "tetrodotoxin/isa/base/function_body.hpp"
#include "ttx/function.hpp"
#include "ttx/member.hpp"

namespace Tetrodotoxin::Isa::Base {

// Implementation is the typed publication table for one evaluated source.
//
// TTX::Function carries stable function signatures. Each body ISA publishes
// its concrete body against that identity, and its lowerer requests that exact
// C++ type. Linkage crosses package boundaries in its own typed table.
// Arbitrary type payloads are deliberately not supported.
//
// Linkages retain an ordered vector because archive and compiler consumers
// enumerate them. A linkage index sits beside that vector for semantic lookup.
// Keeping the two access patterns on one owner avoids repeated scans without
// replacing deterministic publication order with a map iteration contract.
class Implementation {
 public:
  auto define(const Ttx::Function& function, Definition definition = {})
      -> Bool;
  template <typename Body>
  auto define(
      const Ttx::Function& function,
      const Body& body,
      Definition definition = {}) -> Bool {
    return define(function, FunctionBody(body, definition));
  }

  auto define(const Ttx::Member& member, Definition definition) -> Bool;
  auto define(const Ttx::Type& type, Definition definition) -> Bool;
  auto define(Tetrodotoxin::Abi::Linkage linkage) -> Bool;
  template <typename Body>
  auto find(const Ttx::Function& function) const -> const Body* {
    const FunctionBody* entry = find_body(function);
    return entry == nullptr ? nullptr : entry->find<Body>();
  }

  auto find_definition(const Ttx::Function& function) const
      -> const Definition*;
  auto find(const Ttx::Member& member) const -> const Definition*;
  auto find(const Ttx::Type& type) const -> const Definition*;
  auto has(const Ttx::Function& function) const -> Bool;
  auto has(const Ttx::Member& member) const -> Bool;
  auto has(const Ttx::Type& type) const -> Bool;
  auto find_linkage(const Ttx::Function& function) const
      -> const Tetrodotoxin::Abi::Linkage*;
  constexpr auto get_linkages() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> {
    return linkages;
  }

 private:
  auto define(const Ttx::Function& function, FunctionBody body) -> Bool;
  auto find_body(const Ttx::Function& function) const -> const FunctionBody*;

  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Abi::Linkage> linkages;
  Perimortem::Memory::Dynamic::Map<const Ttx::Function*, Count> linkage_indices;
  Perimortem::Memory::Dynamic::Map<const Ttx::Function*, FunctionBody>
      functions;
  Perimortem::Memory::Dynamic::Map<const Ttx::Member*, Definition> members;
  Perimortem::Memory::Dynamic::Map<const Ttx::Type*, Definition> types;
};

}  // namespace Tetrodotoxin::Isa::Base
