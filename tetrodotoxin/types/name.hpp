// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include <string>

namespace Tetrodotoxin::Language::Parser::Types {

class Name : public Abstract {
 public:
  static constexpr uint32_t uuid = 0x61D5666f;
  constexpr auto get_name() const -> std::string_view override { name; };
  constexpr auto get_doc() const -> std::string_view override {
    return "A TTX program including any required libraries.";
  };
  constexpr auto get_uuid() const -> uint32_t override { uuid; };
  constexpr auto get_usage() const -> Usage override { usage; };
  auto get_size() const -> uint32_t override { return resolve()->get_size(); };
  auto resolve() const -> const Abstract* override { type->resolve(); }

  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    resolve()->resolve_context(name);
  }

  // Names are only context and do not own a scope.
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    return nullptr;
  }

  // TODO: We could have names know about their host context but for now this
  // doesn't seem useful so might as well save a little space.
  auto resolve_host() const -> const Abstract* override { return nullptr; }

  virtual auto expand_scope(
      const std::function<void(const Abstract* const)>& fn) const
      -> void override {
    resolve()->expand_context(fn);
  }

  Name(std::string&& doc, std::string_view name, Abstract* type, Usage usage)
      : doc(doc), name(name), type(type), usage(usage) {};

  const std::string doc;
  const std::string name;
  const Abstract* type;
  const Usage usage;
};

}  // namespace Tetrodotoxin::Language::Parser::Types