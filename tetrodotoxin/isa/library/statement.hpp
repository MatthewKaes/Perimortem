// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/library/call.hpp"

namespace Ttx::Lexical {
class Cursor;
}

namespace Tetrodotoxin::Isa::Library {

class Scope;

class Statement {
 public:
  enum class Kind : Bits_8 {
    Empty,
    Return,
    Call,
  };

  class Result;

  constexpr Statement() = default;

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Scope& scope) -> Result;
  static constexpr auto return_statement() -> Statement {
    return Statement(Kind::Return);
  }
  static constexpr auto call_statement(const Call& call) -> Statement {
    return Statement(call);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_call() const -> const Call& { return call; }
  constexpr auto is_empty() const -> Bool { return kind == Kind::Empty; }

 private:
  explicit constexpr Statement(Kind kind) : kind(kind) {}
  explicit constexpr Statement(const Call& call)
      : kind(Kind::Call), call(call) {}

  Kind kind = Kind::Empty;
  Call call;
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

}  // namespace Tetrodotoxin::Isa::Library
