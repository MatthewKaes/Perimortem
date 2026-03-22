// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Types {

class Byt : public Abstract, public Perimortem::Memory::View::Singleton<Byt> {
 public:
  static constexpr uint32_t uuid = 0x586C9460;
  constexpr auto get_name() const -> Perimortem::Memory::View::Bytes override {
    return "Byt";
  };
  constexpr auto get_doc() const -> Perimortem::Memory::View::Bytes override {
    return "A single byte used for manipulating streams or for storing small "
           "values.\n"
           "All values and operations are treated as signed and overflow has "
           "defined behavior.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage override { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(uint8_t); };
};

}  // namespace Tetrodotoxin::Types