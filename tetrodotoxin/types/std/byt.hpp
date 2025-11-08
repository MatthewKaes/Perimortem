// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Language::Parser::Types {

class Byt : public Abstract, public Perimortem::Concepts::Singleton<Abstract> {
 public:
  static constexpr uint32_t uuid = 0x586C9460;
  constexpr auto get_name() const -> std::string_view override {
    return "Byt";
  };
  constexpr auto get_doc() const -> std::string_view override {
    return "A single byte used for manipulating streams or for storing small "
           "values.\n"
           "All values and operations are treated as signed and overflow has "
           "defined behavior.";
  };
  auto get_size() const -> uint32_t override { return sizeof(uint8_t); };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto resolve() const -> const Abstract* override { return this; }
  auto resolve_context(std::string_view name) const
      -> const Abstract* override {
    return nullptr;
  };
  auto resolve_scope(std::string_view name) const -> const Abstract* override {
    return nullptr;
  }
  auto expand_context() const -> std::span<const Abstract* const> override {
    return {};
  }
  auto expand_scope() const -> std::span<const Abstract* const> override {
    return {};
  }
};

}  // namespace Tetrodotoxin::Language::Parser::Types