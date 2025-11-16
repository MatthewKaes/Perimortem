// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include "core/memory/arena.hpp"
#include "core/memory/managed_lookup.hpp"
#include "core/memory/managed_string.hpp"

#include "lexical/tokenizer.hpp"

#include <filesystem>

namespace Tetrodotoxin::Types {

// Assosicates a Path as a Tetrodotoxin
class Library : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xD12AA071;
  constexpr auto get_name() const -> std::string_view override {
    return package_name.get_view();
  };
  constexpr auto get_doc() const -> std::string_view override {
    return doc.get_view();
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr auto get_usage() const -> Usage override {
    return Usage::Transitory;
  };
  auto get_size() const -> uint32_t override { return 0; };
  auto resolve() const -> const Abstract* override { this; }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    return name_index.at(name);
  }

  // The scope of the library is just it's top level context.
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    return name_index.at(name);
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_context(
      const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    name_index.apply(fn);
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_scope(
      const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    expand_context(fn);
  }

  // Trys to create a name. Returns the conflict
  auto create_name(const Perimortem::Memory::ManagedString& name,
                   const Abstract* abstract) -> bool {
    if (name_index.contains(name))
      return false;

    name_index.insert(name, abstract);
    return true;
  }

  inline auto set_name(const Perimortem::Memory::ManagedString& name) -> void {
    package_name = name;
  }

  inline auto set_doc(Perimortem::Memory::ManagedString& doc_string) -> void {
    doc.take(doc_string);
  }

  inline auto set_entity(bool is_entity) -> void {
    this->uses_entity = is_entity;
  }

  inline auto is_entity() const -> bool { return uses_entity; }

  inline auto load(const std::string_view& source, bool strip_disabled) -> void {
    allocator.reset();
    name_index.reset();
    doc.clear();
    package_name.clear();
    uses_entity = false;

    tokenizer.parse(source, strip_disabled);
  }

  Library()
      :  // Ensure allocator is constructed first. Putting it in the initalizer
         // list will catch bugs if it every moves in the class.
        allocator(),
        name_index(allocator),
        doc() {}

 public:
  Perimortem::Memory::Arena allocator;
  Tetrodotoxin::Lexical::Tokenizer tokenizer;

 private:
  Perimortem::Memory::ManagedLookup<const Abstract> name_index;
  Perimortem::Memory::ManagedString doc;
  Perimortem::Memory::ManagedString package_name;
  bool uses_entity;
};

}  // namespace Tetrodotoxin::Types
