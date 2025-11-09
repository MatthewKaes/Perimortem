// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Language::Parser::Types {

class Int : public Abstract, public Perimortem::Concepts::Singleton<Int> {
 public:
  static constexpr uint32_t uuid = 0x586C9468;
  constexpr auto get_name() const -> std::string_view override {
    return "Int";
  };
  constexpr auto get_doc() const -> std::string_view override {
    return "The basic storage type for numeric values. Compiles as a 64 bit "
           "signed integer.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(uint64_t); };
};

}  // namespace Tetrodotoxin::Language::Parser::Types