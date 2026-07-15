// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/isa/base/implementation.hpp"
#include "tetrodotoxin/standard/types.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Base {

// Context is the pushdown state shared by a single ISA evaluation.
//
// It owns the arena used to synthesize durable TTX objects for that source
// transaction and the type names made visible by imports or earlier evaluated
// declarations. It deliberately does not parse type expressions. That grammar
// belongs to Expression::Type.
//
// Temporary scratch data should stay with the cursor or a local container. This
// arena is for objects that must survive as part of the produced TTX model.
// Imported implementation tables remain borrowed producer context: queries can
// see published linkage, but the consumer never copies or republishes it.
class Context {
 public:
  using ReadEmbedded = Bool (*)(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes& content);

  explicit Context(
      Perimortem::Memory::Allocator::Arena& arena,
      Implementation& implementation,
      Perimortem::Core::View::Bytes unit_name = {},
      Perimortem::Core::View::Bytes module = {},
      Perimortem::Core::View::Bytes package_name = {},
      ReadEmbedded read_embedded = nullptr)
      : arena(arena),
        implementation(implementation),
        embedded_reader(read_embedded),
        package_name(package_name),
        unit_name(unit_name),
        module(module) {}

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_unit_name() const -> Perimortem::Core::View::Bytes {
    return unit_name;
  }

  constexpr auto get_module() const -> Perimortem::Core::View::Bytes {
    return module;
  }

  auto read_embedded(
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes& content) -> Bool;

  auto define_type(Perimortem::Core::View::Bytes name, const Ttx::Type& type)
      -> Bool {
    if (types.find(name) != nullptr ||
        Tetrodotoxin::Standard::Types::is_type(name)) {
      return False;
    }

    types.insert(name, &type);
    return True;
  }

  auto define_type(const Ttx::Type& type) -> Bool {
    return define_type(type.get_name(), type);
  }

  // A staged declaration becomes visible with its authored definition. Check
  // both publication tables before either mutation so consumers cannot observe
  // a type name whose implementation fact failed to publish.
  auto define_type(const Ttx::Type& type, Definition definition) -> Bool {
    if (types.find(type.get_name()) != nullptr ||
        Tetrodotoxin::Standard::Types::is_type(type.get_name()) ||
        implementation.has(type)) {
      return False;
    }

    Bool defined_implementation = implementation.define(type, definition);
    if (!defined_implementation) {
      return False;
    }

    types.insert(type.get_name(), &type);
    return True;
  }

  auto find_type(Perimortem::Core::View::Bytes name) const -> const Ttx::Type* {
    const auto* type = types.find(name);
    return type == nullptr ? nullptr : type->value;
  }

  // Parameterization dispatches a type over an unnamed argument layout.
  // Repeated queries return the same concrete identity for this transaction.
  // Extent is the normalized value argument used by fixed-size type families.
  // It does not replace the type-argument layout.
  auto parameterize_type(
      const Ttx::Type& root,
      Ttx::Layout arguments,
      Count extent,
      Perimortem::Core::View::Bytes name) -> const Ttx::Type&;

  auto define_implementation(
      const Ttx::Function& function,
      Definition definition = {}) -> Bool {
    return implementation.define(function, definition);
  }

  template <typename Body>
  auto define_implementation(
      const Ttx::Function& function,
      const Body& body,
      Definition definition = {}) -> Bool {
    return implementation.define(function, body, definition);
  }

  auto define_implementation(const Ttx::Member& member, Definition definition)
      -> Bool {
    return implementation.define(member, definition);
  }

  auto define_implementation(const Ttx::Type& type, Definition definition)
      -> Bool {
    return implementation.define(type, definition);
  }

  auto define_linkage(Tetrodotoxin::Abi::Linkage linkage) -> Bool {
    return implementation.define(linkage);
  }

  // Producers remain alive for the whole evaluation transaction. Retaining
  // their table addresses preserves one owner for each implementation fact.
  auto import_implementation(const Implementation& source) -> void {
    if (!imported_implementations.contains(&source)) {
      imported_implementations.insert(&source);
    }
  }

  auto find_linkage(const Ttx::Function& function) const
      -> const Tetrodotoxin::Abi::Linkage* {
    const auto* linkage = implementation.find_linkage(function);
    if (linkage != nullptr) {
      return linkage;
    }

    for (Count i = 0; i < imported_implementations.get_size(); i++) {
      const auto* linkage = imported_implementations[i]->find_linkage(function);
      if (linkage != nullptr) {
        return linkage;
      }
    }

    return nullptr;
  }

  auto find_definition(const Ttx::Member& member) const -> const Definition* {
    const Definition* definition = implementation.find(member);
    if (definition != nullptr) {
      return definition;
    }

    for (Count i = 0; i < imported_implementations.get_size(); i++) {
      const Definition* definition = imported_implementations[i]->find(member);
      if (definition != nullptr) {
        return definition;
      }
    }

    return nullptr;
  }

 private:
  // Parameterization is Context's interning record, not a second type model.
  // The root identity and argument Layout are the semantic query key.
  class Parameterization {
   public:
    constexpr Parameterization(
        const Ttx::Type& root,
        Ttx::Layout arguments,
        Count extent,
        const Ttx::Type& result)
        : root(root), arguments(arguments), extent(extent), result(result) {}

    auto matches(const Ttx::Type& root, Ttx::Layout arguments, Count extent)
        const -> Bool {
      return &this->root == &root && this->extent == extent &&
             this->arguments.equivalent_to(arguments);
    }

    constexpr auto get_result() const -> const Ttx::Type& { return result; }

   private:
    const Ttx::Type& root;
    Ttx::Layout arguments;
    Count extent;
    const Ttx::Type& result;
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Implementation& implementation;
  ReadEmbedded embedded_reader = nullptr;
  Perimortem::Memory::Dynamic::Vector<const Implementation*>
      imported_implementations;
  Perimortem::Memory::Dynamic::Vector<Parameterization> parameterizations;
  Perimortem::Core::View::Bytes package_name;
  Perimortem::Core::View::Bytes unit_name;
  Perimortem::Core::View::Bytes module;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, const Ttx::Type*>
          types;
};

}  // namespace Tetrodotoxin::Isa::Base
