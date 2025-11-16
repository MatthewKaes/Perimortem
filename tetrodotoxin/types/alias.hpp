// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

namespace Tetrodotoxin::Types {

// Aliases are names that exist in a context.
// Aliases can't reference types outside the package
class Alias : public Abstract {
 public:
  static constexpr uint32_t uuid = 0x812E1DA1;
  constexpr auto get_name() const -> std::string_view override {
    return type->resolve()->get_name();
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage override { return usage; };
  auto get_size() const -> uint32_t override { return 0; };
  auto resolve() const -> const Abstract* override { return type->resolve(); }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    return type->resolve()->resolve_context(name);
  }

  // An alias could redirect scope, but this can get real spicy really
  // quickly, so for now we don't let you alias scopes.
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    // type->resolve()->resolve_scope(name);
    return nullptr;
  }

  auto expand_context(const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    return type->resolve()->expand_context(fn);
  }

  // An alias could redirect scope, but this can get real spicy really quickly,
  // so for now we don't let you alias scopes like functions or blocks.
  auto expand_scope(const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    // type->resolve()->expand_scope();
    return;
  }

  Alias(Abstract* type, Usage usage) : type(type), usage(usage) {};

  const Abstract* type;
  const Usage usage;
};

}  // namespace Tetrodotoxin::Types