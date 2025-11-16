// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Types {

class Num : public Abstract, public Perimortem::Concepts::Singleton<Num> {
 public:
  static constexpr uint32_t uuid = 0x586C9471;
  constexpr auto get_name() const -> std::string_view override {
    return "Num";
  };
  constexpr auto get_doc() const -> std::string_view override {
    return "Stores a 32bit IEEE float. Used for most floating point "
           "operations as well as storage as it's both quick and small.\n"
           "For increased percision you can yoused `Dec`.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(float); };
};

}  // namespace Tetrodotoxin::Types