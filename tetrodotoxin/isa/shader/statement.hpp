// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/expression/pack.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

class Statement {
 public:
  enum class Kind : Bits_8 {
    Empty,
    State,
    Return,
  };

  class Result;

  constexpr Statement() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Context& context) -> Result;
  static constexpr auto state_statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      Perimortem::Core::View::Bytes name,
      const Ttx::Type& type,
      Expression::Value initializer)
      -> Statement {
    Statement statement(Kind::State, start_token, end_token);
    statement.name = name;
    statement.type = &type;
    statement.initializer = initializer;
    return statement;
  }
  static constexpr auto return_statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      const Expression::Pack* pack) -> Statement {
    Statement statement(Kind::Return, start_token, end_token);
    statement.pack = pack;
    return statement;
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_start_token() const -> const Ttx::Lexical::Token* {
    return has_token_range ? &start_token : nullptr;
  }
  constexpr auto get_end_token() const -> const Ttx::Lexical::Token* {
    return has_token_range ? &end_token : nullptr;
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_type() const -> const Ttx::Type* { return type; }
  constexpr auto get_initializer() const -> Expression::Value {
    return initializer;
  }
  constexpr auto get_pack() const -> const Expression::Pack* { return pack; }
  constexpr auto is_empty() const -> Bool { return kind == Kind::Empty; }

 private:
  explicit constexpr Statement(Kind kind) : kind(kind) {}
  constexpr Statement(
      Kind kind,
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token)
      : kind(kind),
        start_token(start_token),
        end_token(end_token),
        has_token_range(True) {}

  Kind kind = Kind::Empty;
  Ttx::Lexical::Token start_token;
  Ttx::Lexical::Token end_token;
  Bool has_token_range = False;
  Perimortem::Core::View::Bytes name;
  const Ttx::Type* type = nullptr;
  Expression::Value initializer;
  const Expression::Pack* pack = nullptr;
};

class Statement::Result {
 public:
  enum class Kind : Bits_8 {
    Failed,
    Ignored,
    Ready,
  };

  constexpr Result() = default;

  static constexpr auto failed() -> Result { return Result(Kind::Failed); }
  static constexpr auto ignored() -> Result { return Result(Kind::Ignored); }
  static constexpr auto ready(Statement statement) -> Result {
    return Result(statement);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_statement() const -> Statement { return statement; }

 private:
  explicit constexpr Result(Kind kind) : kind(kind) {}
  explicit constexpr Result(Statement statement)
      : kind(Kind::Ready), statement(statement) {}

  Kind kind = Kind::Failed;
  Statement statement;
};

}  // namespace Tetrodotoxin::Isa::Shader
