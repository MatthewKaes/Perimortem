// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/union.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/expression/pack.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Statement preserves one authored Shader operation and its source range.
// Empty is reserved for parse failure and never enters a durable Block. Bare
// returns and returns carrying a Pack are distinct authored operations, so
// consumers never infer active state from a nullable payload.
class Statement {
 private:
  class BareReturn {};

  class Return {
   public:
    explicit Return(const Base::Expression::Pack& pack) : pack(pack) {}
    constexpr auto get_pack() const -> const Base::Expression::Pack& {
      return pack;
    }

   private:
    const Base::Expression::Pack& pack;
  };

 public:
  enum class Kind : Bits_8 {
    Empty,
    State,
    BareReturn,
    Return,
  };

  class State {
   public:
    constexpr State(Ttx::Member member, Base::Expression::Value initializer)
        : member(member), initializer(initializer) {}

    constexpr auto get_member() const -> const Ttx::Member& { return member; }
    constexpr auto get_initializer() const -> const Base::Expression::Value& {
      return initializer;
    }

   private:
    Ttx::Member member;
    Base::Expression::Value initializer;
  };

  Statement() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Statement;
  static auto state_statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      Perimortem::Core::View::Bytes name,
      const Ttx::Type& type,
      Base::Expression::Value initializer) -> Statement {
    return Statement(
        start_token, end_token, State(Ttx::Member(name, type), initializer));
  }

  static auto return_statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token) -> Statement {
    return Statement(start_token, end_token, BareReturn());
  }

  static auto return_statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      const Base::Expression::Pack& pack) -> Statement {
    return Statement(start_token, end_token, Return(pack));
  }

  constexpr auto is_empty() const -> Bool { return value.is_null(); }
  constexpr auto get_kind() const -> Kind {
    return value.visit(
        []() -> Kind { return Kind::Empty; },
        [](const State&) -> Kind { return Kind::State; },
        [](const BareReturn&) -> Kind { return Kind::BareReturn; },
        [](const Return&) -> Kind { return Kind::Return; });
  }

  auto get_state() const -> const State& { return *value.find<State>(); }
  auto get_return_pack() const -> const Base::Expression::Pack& {
    return value.find<Return>()->get_pack();
  }

  auto get_start_token() const -> const Ttx::Lexical::Token& {
    return start_token;
  }

  auto get_end_token() const -> const Ttx::Lexical::Token& { return end_token; }

 private:
  Statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      State state)
      : start_token(start_token), end_token(end_token), value(state) {}
  Statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      BareReturn value)
      : start_token(start_token), end_token(end_token), value(value) {}
  Statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      Return value)
      : start_token(start_token), end_token(end_token), value(value) {}

  Ttx::Lexical::Token start_token;
  Ttx::Lexical::Token end_token;
  Perimortem::Core::Static::Union<State, BareReturn, Return> value;
};

}  // namespace Tetrodotoxin::Isa::Shader
