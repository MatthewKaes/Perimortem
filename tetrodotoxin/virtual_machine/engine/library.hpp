// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include "memory/arena.hpp"
#include "memory/view/bytes.hpp"
#include "memory/managed/table.hpp"

#include "lexical/tokenizer.hpp"

#include <filesystem>

namespace Tetrodotoxin::Types {

// Assosicates a Path as a Tetrodotoxin
class Library : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xD12AA071;
  constexpr auto get_name() const -> Perimortem::Memory::View::Bytes override {
    return package_name.get_view();
  };
  constexpr auto get_doc() const -> Perimortem::Memory::View::Bytes override {
    return doc.get_view();
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr auto get_usage() const -> Usage override {
    return Usage::Transitory;
  };
  auto get_size() const -> uint32_t override { return 0; };
  auto resolve() const -> const Abstract* override { return this; }

  auto resolve_context(Perimortem::Memory::View::Bytes name) const
      -> const Abstract* override {
    auto abstract = name_index.at(name);
    if (abstract)
      return *abstract;
    return nullptr;
  }

  // The scope of the library is just it's top level context.
  auto resolve_scope(Perimortem::Memory::View::Bytes name) const -> const Abstract* override {
    auto abstract = name_index.at(name);
    if (abstract)
      return *abstract;
    return nullptr;
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_context(
      const std::function<void(const Perimortem::Memory::View::Bytes&,
                               const Abstract* const&)>& fn)
      const -> void override {
    name_index.apply(fn);
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_scope(
      const std::function<void(const Perimortem::Memory::View::Bytes&,
                               const Abstract* const&)>& fn) const
      -> void override {
    expand_context(fn);
  }

  // Trys to create a name. Returns the conflict
  auto create_name(const Perimortem::Memory::View::Bytes& name,
                   const Abstract& abstract) -> bool {
    if (name_index.contains(name))
      return false;

    name_index.insert(name, &abstract);
    return true;
  }

  inline auto set_name(const Perimortem::Memory::View::Bytes name) -> void {
    package_name = name;
  }

  inline auto set_doc(const Perimortem::Memory::View::Bytes doc_string) -> void {
    doc = doc_string;
  }

  inline auto set_entity(bool is_entity) -> void {
    this->uses_entity = is_entity;
  }

  inline auto is_entity() const -> bool { return uses_entity; }

  inline auto load(const std::string_view& source, bool strip_disabled)
      -> void {
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
  Perimortem::Memory::Managed::Table<const Abstract*> name_index;
  Perimortem::Memory::View::Bytes doc;
  Perimortem::Memory::View::Bytes package_name;
  bool uses_entity;
};

}  // namespace Tetrodotoxin::Types
