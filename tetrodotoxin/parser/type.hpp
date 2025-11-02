// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include <optional>

namespace Tetrodotoxin::Language::Parser {

class Type {
 public:
  enum class Handler : uint8_t {
    Any,   // Stack - Contains type info and fixed data, Boxes: Vec -> List, Str
           // -> Text
    Byt,   // Stack
    Enu,   // Stack
    Flg,   // Stack
    Int,   // Stack
    Num,   // Stack
    Str,   // Stack - Takes Size parameter
    Vec,   // Stack - Takes Size parameter and type
    Text,  // Heap
    List,  // Heap - Generic
    Dict,  // Heap - Generic
    Func,  // Heap - Generic Node Lib
    Type, 
  };

  Type() {};
  Type(Type&& type)
      : handler(type.handler),
        name(type.name),
        parameters(std::move(type.parameters)) {};
  Type& operator=(Type&&) = default;

  static auto parse(Context& ctx) -> std::optional<Type>;
  static auto validate_type(Context& ctx,
                            const Token* start_token,
                            const Token* token,
                            const Type& type) -> bool;

  static constexpr auto uses_stack(Handler handler) -> bool {
    return static_cast<uint32_t>(handler) <=
           static_cast<uint32_t>(Handler::Any);
  }

  Handler handler = Handler::Any;
  std::string_view name;
  std::unique_ptr<std::vector<Type>> parameters;
};

}  // namespace Tetrodotoxin::Language::Parser
