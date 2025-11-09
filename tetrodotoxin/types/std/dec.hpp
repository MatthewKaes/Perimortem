// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "concepts/singleton.hpp"

namespace Tetrodotoxin::Language::Parser::Types {

class Dec : public Abstract, public Perimortem::Concepts::Singleton<Dec> {
 public:
  static constexpr uint32_t uuid = 0x586C9444;
  constexpr auto get_name() const -> std::string_view override {
    return "Dec";
  };
  constexpr auto get_doc() const -> std::string_view override {
    return "Stores a 128bit IEEE float. `Dec` should only be used for "
           "situations that require extreme precision.\n"
           "For long term storage and most regular cases use `Num` instead.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(long double); };
};

}  // namespace Tetrodotoxin::Language::Parser::Types