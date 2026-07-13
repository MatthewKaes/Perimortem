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
// State owns declaration facts while the return alternative owns an expression
// pack; inactive payloads are never carried into the durable Block.
class Statement {
 public:
  class State {
   public:
    constexpr State(Ttx::Member member, Base::Expression::Value initializer)
        : member(member), initializer(initializer) {}

    constexpr auto get_member() const -> const Ttx::Member& { return member; }
    constexpr auto get_initializer() const -> Base::Expression::Value {
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
      const Ttx::Lexical::Token& end_token,
      const Base::Expression::Pack* pack) -> Statement {
    return Statement(start_token, end_token, pack);
  }

  constexpr auto is_empty() const -> Bool { return value.is_null(); }
  auto find_state() const -> const State* { return value.find<State>(); }
  auto is_return() const -> Bool {
    return value.find<const Base::Expression::Pack*>() != nullptr;
  }

  auto get_return_pack() const -> const Base::Expression::Pack* {
    const auto* pack = value.find<const Base::Expression::Pack*>();
    return pack == nullptr ? nullptr : *pack;
  }

  auto get_start_token() const -> const Ttx::Lexical::Token* {
    return is_empty() ? nullptr : &start_token;
  }

  auto get_end_token() const -> const Ttx::Lexical::Token* {
    return is_empty() ? nullptr : &end_token;
  }

 private:
  Statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      State state)
      : start_token(start_token), end_token(end_token), value(state) {}
  Statement(
      const Ttx::Lexical::Token& start_token,
      const Ttx::Lexical::Token& end_token,
      const Base::Expression::Pack* pack)
      : start_token(start_token), end_token(end_token), value(pack) {}

  Ttx::Lexical::Token start_token;
  Ttx::Lexical::Token end_token;
  Perimortem::Core::Static::Union<State, const Base::Expression::Pack*> value;
};

}  // namespace Tetrodotoxin::Isa::Shader
