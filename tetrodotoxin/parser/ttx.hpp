// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/comment.hpp"
#include "parser/ast/context.hpp"
#include "parser/ast/init.hpp"
#include "parser/error.hpp"
#include "parser/tokenizer.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace Tetrodotoxin::Language::Parser {

class Ttx {
 public:
  class Dependency {
   public:
    std::string path;
    std::string type;
  };

  static auto parse(const std::string_view& source_map, const ByteView& source)
      -> std::unique_ptr<Ttx>;
  inline auto get_errors() const -> const Errors& { return errors; }

  std::string name;
  std::optional<Comment> documentation;
  std::unordered_map<std::string, std::string> config;
  std::optional<Init> __init;
  const std::string source_map;

 private:
  Ttx(const std::string_view& source_map, const ByteView& source)
      : source_map(source_map), source(source.begin(), source.end()) {};
  auto parse_header(Context& ctx) -> void;

  const Bytes source;
  Errors errors;
};

}  // namespace Tetrodotoxin::Language::Parser
