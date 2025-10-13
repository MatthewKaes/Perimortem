// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/sparse_lookup.hpp"
#include "parser/ast/context.hpp"

#include <optional>

using namespace Perimortem::Concepts;

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


  static constexpr auto uses_stack(Handler handler) -> bool {
    return static_cast<uint32_t>(handler) <= static_cast<uint32_t>(Handler::Any);
  }

  Handler handler = Handler::Any;
  std::string_view name;
  std::unique_ptr<std::vector<Type>> parameters;
};

namespace HandlerTable {
using value_type = Type::Handler;
static constexpr auto sparse_factor = 120;
static constexpr auto seed = 5793162292815167211UL;
static constexpr TablePair<const char*, value_type> data[] = {
    make_pair("Byt", value_type::Byt),   make_pair("Int", value_type::Int),
    make_pair("Num", value_type::Num),   make_pair("Str", value_type::Str),
    make_pair("Vec", value_type::Vec),   make_pair("Any", value_type::Any),
    make_pair("Text", value_type::Text), make_pair("List", value_type::List),
    make_pair("Dict", value_type::Dict), make_pair("Func", value_type::Func),
};

using lookup = SparseLookupTable<value_type,
                                          array_size(data),
                                          data,
                                          sparse_factor,
                                          seed>;

static_assert(lookup::has_perfect_hash());
static_assert(sizeof(lookup::sparse_table) <= 512,
              "Ideally keyword sparse table would be less than 512 bytes. "
              "Try to find a smaller size.");
}

}  // namespace Tetrodotoxin::Language::Parser
