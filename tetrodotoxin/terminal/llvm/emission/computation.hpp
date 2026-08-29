// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "ttx/bootstrap/concept/layout.hpp"
#include "ttx/bootstrap/model/addressable.hpp"
#include "ttx/bootstrap/model/callable.hpp"
#include "ttx/bootstrap/model/type.hpp"
#include "ttx/lexical/anchor.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

// Computation owns arithmetic and comparison instructions after Library fixes
// exact operand Types.
class Computation {
 public:
  enum class Arithmetic : U8 {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
  };

  enum class Comparison : U8 {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
  };

  constexpr Computation(Module::Body& body) : body(body) {}
  Computation(const Computation&) = delete;
  Computation(Computation&&) = delete;
  auto operator=(const Computation&) -> Computation& = delete;
  auto operator=(Computation&&) -> Computation& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto arithmetic(
      Arithmetic operation,
      const Ttx::Model::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& left,
      const Tetrodotoxin::Library::Language::Model::Pack& right) const -> Bool;
  auto negate(
      const Ttx::Model::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& operand) const
      -> Bool;
  auto convert(
      const Ttx::Model::Type& source_carrier,
      const Ttx::Model::Type& target_carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& source) const -> Bool;
  auto compare(
      Comparison operation,
      const Ttx::Model::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& left,
      const Tetrodotoxin::Library::Language::Model::Pack& right) const -> Bool;
  auto compare_bytes(
      Comparison operation,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& left,
      const Tetrodotoxin::Library::Language::Model::Pack& right) const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
