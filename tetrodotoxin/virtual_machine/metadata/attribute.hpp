// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "core/memory/view/byte.hpp"

namespace Tetrodotoxin::Types::Compiler {

class Attribute : public Abstract {
 public:
  static constexpr uint32_t uuid = 0x1772fAAE;
  constexpr auto get_name() const -> std::string_view override {
    return name.get_view();
  };
  constexpr auto get_doc() const -> std::string_view override {
    return doc.get_view();
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; };
  constexpr virtual auto get_usage() const -> Usage { return Usage::Constant; };
  auto get_size() const -> uint32_t override { return sizeof(uint8_t); };

  View::Bytes name;
  View::Bytes value;
  View::Bytes doc;
};

}  // namespace Tetrodotoxin::Types::Compiler