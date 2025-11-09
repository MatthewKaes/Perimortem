// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include "core/memory/arena.hpp"
#include "core/memory/managed_string.hpp"

#include <filesystem>
#include <unordered_map>

namespace Tetrodotoxin::Language::Parser::Types {

// Typically the file
class Library : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xD12AA071;
  constexpr auto get_name() const -> std::string_view override {
    return source_map.get_view();
  };
  constexpr auto get_doc() const -> std::string_view override { return doc.get_view(); };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr auto get_usage() const -> Usage override {
    return Usage::Transitory;
  };
  auto get_size() const -> uint32_t override { return 0; };
  auto resolve() const -> const Abstract* override { this; }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    if (!name_index.contains(name))
      return nullptr;

    return name_index.at(name);
  }

  // The scope of the library is just it's top level context.
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    if (!name_index.contains(name))
      return host.resolve_scope(name);

    return name_index.at(name);
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_context(
      std::function<void(const Abstract* const)> fn) const -> void override {
    for (const auto& named_pair : name_index) {
      fn(named_pair.second);
    }
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_scope(std::function<void(const Abstract* const)> fn) const
      -> void override {
    expand_context(fn);
  }

  auto create_name(std::string_view name, Abstract* abstract) -> bool {
    if (name_index.contains(name))
      return false;

    name_index[name] = abstract;
    return true;
  }

  Library(const Abstract& host,
          std::string_view doc,
          std::filesystem::path source_map,
          bool is_entity)
      : host(host),
        doc(allocator, doc),
        source_map(allocator, source_map.c_str()),
        is_entity(is_entity) {}

 private:
  Perimortem::Memory::Arena allocator;
  std::unordered_map<std::string_view, const Abstract*> name_index;

 public:
  const Abstract& host;
  const Perimortem::Memory::ManagedString doc;
  const Perimortem::Memory::ManagedString source_map;
  const bool is_entity;
};

}  // namespace Tetrodotoxin::Language::Parser::Types
