// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include "types/func.hpp"
#include "types/library.hpp"
#include "types/name.hpp"

#include <filesystem>
#include <unordered_map>

namespace Tetrodotoxin::Types {

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

    return path_registry.at(name).get();
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
  auto resolve_host() const -> const Abstract* override { return nullptr; }

  auto expand_context(const std::function<void(const Abstract* const)>& fn)
      const -> void override {
    // Include all types compiled from path.
    for (const auto& named_pair : path_registry) {
      fn(named_pair.second.get());
    }
    // Include all external types.
    for (const auto& named_pair : external_abstracts) {
      fn(named_pair.second);
    }
  }

  // Declares a scope level name.
  // Note: Program does not take ownership of these objects as they are
  // expected to either have static linkage, or the injected logic will
  // outlive the program.
  auto declare_external(const Abstract* abstract) -> bool {
    if (external_abstracts.contains(abstract->get_name()))
      return false;

    external_abstracts[abstract->get_name()] = abstract;
  }

  // Loads a compile unit to a path name. If the compile unit already exists
  // then it will be unloaded and the new one will be used.
  auto create_compile_unit(std::filesystem::path name) -> Library& {
    name = std::filesystem::absolute(name);
    if (path_registry.contains(name))
      return *path_registry.at(name);

    path_registry[name] = std::make_unique<Library>();
    return *path_registry[name];
  }

  // There are a lot of tests and other entry points that can create program.
  // Since it's extremely common to want to include the standard library for
  // compilation the logic for including it is centralized here as a context.
  Program(bool include_std_lib = true);

 private:
  std::unordered_map<std::string_view, const Abstract*> external_abstracts;
  std::unordered_map<std::filesystem::path, std::unique_ptr<Library>>
      path_registry;
};

}  // namespace Tetrodotoxin::Types
