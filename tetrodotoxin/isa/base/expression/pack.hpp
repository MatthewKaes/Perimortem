// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "ttx/layout.hpp"

namespace Tetrodotoxin::Isa::Base::Expression {

class Pack {
 public:
  class Entry {
   public:
    constexpr Entry() = default;
    constexpr Entry(Perimortem::Core::View::Bytes name, Value value)
        : name(name), value(value) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    constexpr auto get_value() const -> Value { return value; }
    constexpr auto is_named() const -> Bool { return !name.is_empty(); }

   private:
    Perimortem::Core::View::Bytes name;
    Value value;
  };

  constexpr Pack() = default;
  explicit constexpr Pack(Perimortem::Core::View::Vector<Value> values)
      : values(values) {}
  constexpr Pack(
      Perimortem::Core::View::Vector<Entry> entries,
      Perimortem::Core::View::Vector<Value> values)
      : entries(entries), values(values) {}

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context)
      -> const Pack*;

  // A pack owns the authored field names and carrier order, but expression
  // typing owns the value types. Once the caller has resolved the values this
  // projects the pack into the shared TTX Layout model so calls, returns, and
  // construction use the same fit rules.
  auto schema(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Ttx::Member> value_schema) const
      -> Ttx::Layout;

  constexpr auto get_entries() const -> Perimortem::Core::View::Vector<Entry> {
    return entries;
  }

  constexpr auto get_values() const -> Perimortem::Core::View::Vector<Value> {
    return values;
  }

 private:
  Perimortem::Core::View::Vector<Entry> entries;
  Perimortem::Core::View::Vector<Value> values;
};

}  // namespace Tetrodotoxin::Isa::Base::Expression
