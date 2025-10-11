// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/context.hpp"

#include <optional>

namespace Tetrodotoxin::Language::Parser {

class Type {
 public:
  enum class Handler {
    Byt,      // Stack - 8 byts
    Int,      // Stack
    Num,      // Stack
    Vec,      // Stack - Takes Size parameter
    Defined,  // Class - Could be Stack or Heap depending TODO: Split hanlder
    Text,     // Heap
    List,     // Heap - Generic
    Dict,     // Heap - Generic
    Func,     // Heap - Generic
    Any,      // Heap - Top Type (Boxed)
    // Tetrodotoxin doesn't support extending paramaterized types and has no
    // Unit type.

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

#define PARSE_HANDLER(type_name, handler_name)                           \
  case #type_name[0]: {                                                  \
    if (name.size() == sizeof(#type_name) - 1 &&                         \
        !std::memcmp(name.data(), #type_name, sizeof(#type_name) - 1)) { \
      return Handler::handler_name;                                      \
    } else {                                                             \
      return Handler::Defined;                                           \
    }                                                                    \
  }

#define PARSE_CONCRETE(type_name) PARSE_HANDLER(type_name, type_name)

#define PARSE_GENERIC(type_name) PARSE_HANDLER(type_name, type_name)

  static constexpr auto detect_handler(const std::string_view& name)
      -> Handler {
    switch (name[0]) {
      // Stack types (Always 3 characters)
      PARSE_CONCRETE(Byt);
      PARSE_CONCRETE(Int);
      PARSE_CONCRETE(Num);
      PARSE_CONCRETE(Vec);

      // Heap Types (Always 4 character)
      PARSE_GENERIC(Text);
      // Generics
      PARSE_GENERIC(Dict);
      PARSE_GENERIC(List);
      PARSE_GENERIC(Func);
    }

    return Handler::Defined;
  }

  static constexpr auto uses_stack(Handler handler) -> bool {
    switch (handler) {
      case Handler::Byt:
      case Handler::Int:
      case Handler::Num:
      case Handler::Vec:
        return true;
      default:
        return false;
    }
  }

  Handler handler = Handler::Any;
  std::string_view name;
  std::unique_ptr<std::vector<Type>> parameters;
};

}  // namespace Tetrodotoxin::Language::Parser
