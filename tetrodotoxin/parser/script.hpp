// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/error.hpp"
#include "lexical/tokenizer.hpp"

#include "types/library.hpp"
#include "types/program.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace Tetrodotoxin::Parser {

class Script {
 public:
  Script(bool optimize) : optimize(optimize) {}
  auto parse(Types::Program& host,
             Errors& errors,
             const std::filesystem::path& source_map,
             const std::string_view& source) -> Types::Library&;

 private:
  bool optimize;
};

}  // namespace Tetrodotoxin::Parser
