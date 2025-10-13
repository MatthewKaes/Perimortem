// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/context.hpp"

#include <optional>

namespace Tetrodotoxin::Language::Parser {

class Type {
 public:
  enum class Handler {
    Byt,  // Stack - 8 byts
    Int,  // Stack
    Num,  // Stack
    Str,  // Stack - Takes Size parameter
    Vec,  // Stack - Takes Size parameter and type
    Any,  // Stack - Contains type info and fixed data, Vec -> List, Str -> Text
    Text,     // Heap
    List,     // Heap - Generic
    Dict,     // Heap - Generic
    Func,     // Heap - Generic
    Defined,  // Class - Could be Stack or Heap depending TODO: Split hanlder
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

  static constexpr auto detect_handler(const std::string_view& name)
      -> Handler {
    switch (name.size()) {
      case 3: {
        // Stack types (Always 3 characters)
        if (!std::memcmp(name.data(), "Byt", sizeof("Byt") - 1))
          return Handler::Byt;

        if (!std::memcmp(name.data(), "Int", sizeof("Int") - 1))
          return Handler::Int;

        if (!std::memcmp(name.data(), "Num", sizeof("Num") - 1))
          return Handler::Num;

        if (!std::memcmp(name.data(), "Vec", sizeof("Vec") - 1))
          return Handler::Vec;
        break;

        if (!std::memcmp(name.data(), "Str", sizeof("Str") - 1))
          return Handler::Str;
        break;

        if (!std::memcmp(name.data(), "Any", sizeof("Any") - 1))
          return Handler::Any;
      } break;

      case 4: {
        // Heap Types (Always 4 character)
        if (!std::memcmp(name.data(), "Text", sizeof("Text") - 1))
          return Handler::Byt;

        if (!std::memcmp(name.data(), "Dict", sizeof("Dict") - 1))
          return Handler::Int;

        if (!std::memcmp(name.data(), "List", sizeof("List") - 1))
          return Handler::Num;

        if (!std::memcmp(name.data(), "Func", sizeof("Func") - 1))
          return Handler::Vec;
      } break;

      default:
        break;
    }

    return Handler::Defined;
  }

  static constexpr auto uses_stack(Handler handler) -> bool {
    switch (handler) {
      case Handler::Byt:
      case Handler::Int:
      case Handler::Num:
      case Handler::Vec:
      case Handler::Str:
      case Handler::Any:
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
