// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/error.hpp"
#include "parser/tokenizer.hpp"

#include "types/program.hpp"
#include "types/library.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace Tetrodotoxin::Language::Parser {

class Script {
 public:
  static auto parse(Types::Program& host,
                    Errors& errors,
                    const std::filesystem::path& source_map,
                    Tokenizer& tokenizer) -> Types::Library*;
};

}  // namespace Tetrodotoxin::Language::Parser
