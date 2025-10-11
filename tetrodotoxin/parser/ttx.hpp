// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/comment.hpp"
#include "parser/ast/context.hpp"
#include "parser/ast/init.hpp"
#include "parser/error.hpp"
#include "parser/tokenizer.hpp"

#include <memory>

namespace Tetrodotoxin::Language::Parser {

class Ttx {
 public:
  class Dependency {
   public:
    std::string path;
    std::string type;
  };

  enum class Type { None, Library, Object };

  Ttx(const std::string_view& source_map, const ByteView& source);

  inline auto get_errors() const -> const Errors& { return errors; }

  std::string name;
  std::optional<Comment> documentation;
  std::optional<Init> __init;
  Type type = Type::None;
  const std::string source_map;

 private:
  auto parse_header(Context& ctx) -> void;

  const Bytes source;
  Errors errors;
};

}  // namespace Tetrodotoxin::Language::Parser
