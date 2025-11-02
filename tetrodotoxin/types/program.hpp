// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/name.hpp"

#include <filesystem>
#include <unordered_map>

namespace Tetrodotoxin::Language::Parser::Types {

// Typically the file
class Program : public Abstract {
 public:
  static constexpr uint32_t uuid = 0x00000001;
  constexpr auto get_name() const -> std::string_view override {
    return "ttx_main";
  };
  constexpr auto get_doc() const -> std::string_view override {
    return "A TTX program including any required libraries.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr auto get_usage() const -> Usage override {
    return Usage::Transitory;
  };
  auto get_size() const -> uint32_t override { return 0; };
  auto resolve() const -> const Abstract* override { this; }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    if (!path_registry.contains(name))
      return nullptr;

    return path_registry.at(name);
  }

  // Program links any external to TTX symbols. This can in practice be viewed
  // as more or less the global namespace.
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    if (external_abstracts.contains(name))
      return external_abstracts.at(name);

    return nullptr;
  }

  // Program assumes it is the top most scope, but theoretically it could be
  // extended to host submodules if we ever want 
  auto resolve_host() const -> const Abstract* override {
    return nullptr;
  }

  auto expand_context() const -> std::span<const Abstract* const> {
    return module_table;
  }

  // Program doesn't own any scopes. It only serves as a host context.
  auto expand_scope() const -> std::span<const Abstract* const> override {
    return {};
  }

  // Declares a scope level name.
  // Note: Program does not take ownership of these objects as they are
  // expected to either have static linkage, or the injected logic will
  // outlive the program.
  auto declare_external(std::string_view name, const Abstract* abstract)
      -> bool {
    if (external_abstracts.contains(name))
      return false;

    external_abstracts[name] = abstract;
  }

  auto create_compile_unit(std::filesystem::path name,
                           Abstract* abstract_to_own) -> bool {
    if (path_registry.contains(name))
      return false;

    path_registry[name] = abstract_to_own;
    module_table.push_back(abstract_to_own);
    compile_units.emplace_back(abstract_to_own);
    return true;
  }

 private:
  std::unordered_map<std::string_view, const Abstract*> external_abstracts;
  std::vector<const Abstract*> module_table;
  std::vector<std::unique_ptr<Abstract>> compile_units;
  std::unordered_map<std::filesystem::path, const Abstract*> path_registry;
};

}  // namespace Tetrodotoxin::Language::Parser::Types
