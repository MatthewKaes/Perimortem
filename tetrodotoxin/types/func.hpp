// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/name.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Tetrodotoxin::Types {

class Func : public Abstract {
 public:
  static constexpr uint32_t uuid = 0xFB689410;
  constexpr auto get_name() const -> std::string_view override { return name; };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr auto get_usage() const -> Usage override { return usage; };
  auto get_size() const -> uint32_t override {
    uint32_t stack_size = 0;
    for (const auto& local : scope_variables) {
      stack_size += local->get_size();
    }

    return stack_size;
  };
  auto resolve() const -> const Abstract* override { return_type.resolve(); }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    return_type.resolve_context(name);
  }

  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    if (name_index.contains(name))
      return name_index.at(name);

    // Bubble up to parent scope for resolution.
    return host.resolve_scope(name);
  }

  auto expand_context(const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    return_type.resolve()->expand_context(fn);
  }

  // Return all references in this scope (locals and arguments)
  virtual auto expand_scope(const std::function<void(const Abstract* const)>& fn)
      const -> void override {
    for (const auto& named_pair : name_index) {
      fn(named_pair.second);
    }
  }

  Func(std::string&& doc,
       const Abstract& host,
       std::string_view name,
       const Abstract& return_type,
       Usage usage,
       std::vector<std::unique_ptr<Name>>&& args)
      : host(host),
        name(name),
        return_type(return_type),
        usage(usage),
        args(std::move(args)) {
    for (const auto& arg : args) {
      name_index[arg->name] = arg.get();
    }
  };

  const Abstract& host;
  const std::string doc;
  const std::string name;
  const Abstract& return_type;
  const Usage usage;
  const std::vector<std::unique_ptr<Name>> args;

 private:
  std::vector<std::unique_ptr<Abstract>> scope_variables;
  std::unordered_map<std::string_view, const Abstract*> name_index;
};

}  // namespace Tetrodotoxin::Types