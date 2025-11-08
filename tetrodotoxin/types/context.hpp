// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"
#include <memory>

namespace Tetrodotoxin::Language::Parser::Types {

// Aliases are names that exist in a context.
// Aliases can't reference types outside the package
class Context {
 public:
  using size_type = uint16_t;
  auto get_size() const -> size_type;
  auto push_abstract(std::string_view name, const Abstract* abstract) const
      -> void;

 private:
  struct Block {
    std::string_view name;
    const Abstract* abstract;
  };

  static const constexpr uint32_t size = sizeof(std::string_view);
  static const constexpr uint32_t size2 = sizeof(Block*);

  size_type size;
  size_type capacity;
};
struct Iterator {
  Iterator(Party* ptr, size_t idx) : Party(ptr), idx(idx) {}

  Player& operator*() const {
    if (idx == 0)
      return Party->A;
    if (idx == 1)
      return Party->B;
    if (idx == 2)
      return Party->C;

    throw std::out_of_range{"Parties can only have 3 players"};
  }

  Iterator& operator++() {
    idx++;
    return *this;
  }

  bool operator==(const Iterator& b) const {
    return Party == b.Party && idx == b.idx;
  }

 private:
  size_t idx;
  Party* Party;
};

}  // namespace Tetrodotoxin::Language::Parser::Types