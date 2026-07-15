// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/app/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/layout/evaluator.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto App::VirtualMachine::evaluate_function(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation) -> Ttx::Function {
  Bool has_function =
      cursor.require(Class::Type::Func, "Expected `func` in App."_view);
  if (!has_function) {
    return Ttx::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected App function name."_view);
  if (name == nullptr) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> parameters(context.get_arena());
  Bool parameters_evaluated =
      Base::Layout::Evaluator::evaluate_bracketed(cursor, context, parameters);
  if (!parameters_evaluated) {
    return Ttx::Function();
  }

  Bool has_call = cursor.require(
      Class::Type::CallOp, "Expected `->` before App function result."_view);
  if (!has_call) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(context.get_arena());
  Bool result_evaluated =
      Base::Layout::Evaluator::evaluate(cursor, context, result);
  if (!result_evaluated) {
    return Ttx::Function();
  }

  Bool body_consumed = Base::Expression::Evaluator::consume_block(
      cursor, "Expected `{` after App function signature."_view,
      "Expected `}` after App function body."_view);
  if (!body_consumed) {
    return Ttx::Function();
  }

  return Ttx::Function(
      name->get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
}

auto App::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Bool attributes_consumed = Base::Attribute::consume_all(cursor);
    if (!attributes_consumed) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    Bool is_public = cursor.require(
        Class::Type::Public, "Expected public App `main` function."_view);
    if (!is_public) {
      return nullptr;
    }

    Ttx::Function function = evaluate_function(cursor, context, documentation);
    if (function.is_empty()) {
      return nullptr;
    }

    if (function.get_name() != "main"_view) {
      cursor.token_error("App root can only define `main`."_view);
      valid = False;
    }

    if (function.get_parameters().get_member_count() != 0 ||
        function.get_result().get_member_count() != 0) {
      cursor.token_error("App `main` must use `main[] -> []`."_view);
      valid = False;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("App function name is already defined."_view);
        valid = False;
      }
    }

    if (valid) {
      functions.insert(function);
    }
  }

  if (!valid || functions.get_size() != 1) {
    if (functions.get_size() == 0 && cursor.get_errors().is_empty()) {
      cursor.error("App source must define public `main[] -> []`."_view);
    }

    return nullptr;
  }

  Bool implementation_defined = context.define_implementation(
      functions[0], Base::Definition(Class::Type::Public));
  if (!implementation_defined) {
    return nullptr;
  }

  return &context.get_arena().construct<Ttx::Type>(
      App::VirtualMachine::get_name(), View::Vector<Ttx::Member>(),
      View::Vector<Ttx::Type::Reference>(), functions.get_view());
}
