// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/linkage.hpp"
#include "tetrodotoxin/isa/base/definition.hpp"
#include "tetrodotoxin/isa/base/function_body.hpp"
#include "ttx/function.hpp"
#include "ttx/member.hpp"

namespace Tetrodotoxin::Isa::Base {

// Implementation is the typed publication table for one evaluated source.
//
// TTX::Function carries stable function signatures. Each body ISA publishes
// its concrete body against that identity, and its lowerer requests that exact
// C++ type. Linkage crosses package boundaries in its own typed table;
// arbitrary type payloads are deliberately not supported.
class Implementation {
 public:
  auto define(const Ttx::Function& function, Definition definition = {})
      -> Bool;
  template <typename Body>
  auto define(
      const Ttx::Function& function,
      const Body& body,
      Definition definition = {}) -> Bool {
    return define(function, &body, &body_type<Body>, definition);
  }

  auto define(const Ttx::Member& member, Definition definition) -> Bool;
  auto define(Tetrodotoxin::Compiler::Linkage linkage) -> Bool;
  template <typename Body>
  auto find(const Ttx::Function& function) const -> const Body* {
    const FunctionBody* entry = find_body(function);
    if (entry == nullptr || entry->get_type() != &body_type<Body>) {
      return nullptr;
    }

    return static_cast<const Body*>(entry->get_value());
  }

  auto find_definition(const Ttx::Function& function) const
      -> const Definition*;
  auto find(const Ttx::Member& member) const -> const Definition*;
  auto has(const Ttx::Function& function) const -> Bool;
  auto has(const Ttx::Member& member) const -> Bool;
  auto find_linkage(const Ttx::Function& function) const
      -> const Tetrodotoxin::Compiler::Linkage*;
  constexpr auto get_linkages() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Compiler::Linkage> {
    return linkages;
  }

 private:
  template <typename Body>
  inline static constexpr Bits_8 body_type = 0;
  auto define(
      const Ttx::Function& function,
      const void* body,
      const void* type,
      Definition definition) -> Bool;
  auto find_body(const Ttx::Function& function) const -> const FunctionBody*;

  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Compiler::Linkage> linkages;
  Perimortem::Memory::Dynamic::Map<const Ttx::Function*, FunctionBody>
      functions;
  Perimortem::Memory::Dynamic::Map<const Ttx::Member*, Definition> members;
};

}  // namespace Tetrodotoxin::Isa::Base
