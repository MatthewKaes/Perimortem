// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include "core/data/arena.hpp"

#include <filesystem>
#include <unordered_map>

namespace Tetrodotoxin::Language::Parser::Types {

// Typically the file
class Library : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xD12AA071;
  constexpr auto get_name() const -> std::string_view override {
    return source_map.c_str();
  };
  constexpr auto get_doc() const -> std::string_view override { return doc; };
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
  virtual auto expand_context(std::function<void(const Abstract* const)> fn) const
      -> void override {
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

  Library(std::string&& doc,
          const Abstract& host,
          std::filesystem::path source_map,
          bool is_entity)
      : doc(doc), host(host), source_map(source_map), is_entity(is_entity) {}

  const Abstract& host;
  const std::string doc;
  const std::filesystem::path source_map;
  const bool is_entity;

 private:
  std::unordered_map<std::string_view, const Abstract*> name_index;
  Perimortem::Data::Arena allocator;
};

}  // namespace Tetrodotoxin::Language::Parser::Types
