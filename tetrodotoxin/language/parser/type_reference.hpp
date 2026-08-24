// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/type_reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

class TypeReference {
 public:
  TypeReference() = delete;

  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>;
};

}  // namespace Tetrodotoxin::Language::Parser
