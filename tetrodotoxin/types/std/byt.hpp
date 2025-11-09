// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Language::Parser::Types {

class Byt : public Abstract, public Perimortem::Concepts::Singleton<Byt> {
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
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(uint8_t); };
};

}  // namespace Tetrodotoxin::Language::Parser::Types