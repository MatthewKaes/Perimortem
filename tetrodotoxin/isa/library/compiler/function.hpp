// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/compiler/execution/builder.hpp"
#include "tetrodotoxin/isa/base/definition.hpp"
#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library::Compiler {

// Compiles one Library body directly into compiler operations. No Library
// execution nodes survive this boundary.
class Function {
 public:
  static auto compile(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Ttx::Function& function,
      Perimortem::Utility::Range source)
      -> const Tetrodotoxin::Compiler::Execution::Body*;
  static auto publish(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Ttx::Type& owner,
      Perimortem::Core::View::Vector<Perimortem::Utility::Range> sources,
      Perimortem::Core::View::Vector<Tetrodotoxin::Isa::Base::Definition>
          definitions) -> Bool;

 private:
  Function(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Ttx::Function& function);

  auto build() -> const Tetrodotoxin::Compiler::Execution::Body*;
  auto lower(
      const Tetrodotoxin::Isa::Base::Expression::Value& value,
      const Ttx::Type& expected) -> Tetrodotoxin::Compiler::Execution::Operand;
  auto lower_reference(
      const Tetrodotoxin::Isa::Base::Expression::Value& value,
      const Ttx::Type& expected) -> Tetrodotoxin::Compiler::Execution::Operand;
  auto lower_call(const Tetrodotoxin::Isa::Base::Expression::Value& value)
      -> Tetrodotoxin::Compiler::Execution::Operand;
  auto emit_call(const Tetrodotoxin::Isa::Base::Expression::Value& value)
      -> Perimortem::Utility::Range;
  auto emit_call(
      const Ttx::Function& target,
      const Tetrodotoxin::Compiler::Linkage& linkage,
      const Tetrodotoxin::Isa::Base::Expression::Pack& arguments)
      -> Perimortem::Utility::Range;
  auto evaluate_return() -> Bool;
  auto evaluate_call() -> Bool;
  auto resolve_type(const Tetrodotoxin::Isa::Base::Expression::Value& value)
      const -> const Ttx::Type*;
  auto infer_type(const Tetrodotoxin::Isa::Base::Expression::Value& value) const
      -> const Ttx::Type*;
  auto literal_fits(
      const Tetrodotoxin::Isa::Base::Expression::Value& value,
      const Ttx::Type& expected) const -> Bool;
  auto report_missing_parameter(
      const Tetrodotoxin::Isa::Base::Expression::Value& value) -> Bool;

  Ttx::Lexical::Cursor& cursor;
  Scope& scope;
  const Ttx::Function& function;
  Tetrodotoxin::Compiler::Execution::Builder builder;
};

}  // namespace Tetrodotoxin::Isa::Library::Compiler
