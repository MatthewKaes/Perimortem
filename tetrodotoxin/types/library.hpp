// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include "core/memory/arena.hpp"
#include "core/memory/managed_lookup.hpp"
#include "core/memory/managed_string.hpp"

#include <filesystem>

namespace Tetrodotoxin::Language::Parser::Types {

// Typically the file
class Library : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xD12AA071;
  constexpr auto get_name() const -> std::string_view override {
    return source_map.get_view();
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

  auto create_name(std::string_view name, Abstract* abstract) -> bool {
    if (name_index.contains(name))
      return false;

    name_index.insert(name, abstract);
    return true;
  }

  Library(const Abstract& host,
          std::string_view doc,
          std::filesystem::path source_map,
          bool is_entity)
      : name_index(allocator),
        host(host),
        doc(allocator, doc),
        source_map(allocator, source_map.c_str()),
        is_entity(is_entity) {}

 private:
  Perimortem::Memory::Arena allocator;
  Perimortem::Memory::ManagedLookup<Abstract> name_index;

 public:
  const Abstract& host;
  const Perimortem::Memory::ManagedString doc;
  const Perimortem::Memory::ManagedString source_map;
  const bool is_entity;
};

}  // namespace Tetrodotoxin::Language::Parser::Types
