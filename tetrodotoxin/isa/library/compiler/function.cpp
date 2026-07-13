// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/compiler/function.hpp"

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/compiler/linkage.hpp"
#include "tetrodotoxin/diagnostics/suggestions.hpp"
#include "tetrodotoxin/isa/base/expression/pack.hpp"
#include "tetrodotoxin/standard/types.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

Library::Compiler::Function::Function(
    Cursor& cursor,
    Scope& scope,
    const Ttx::Function& function)
    : cursor(cursor),
      scope(scope),
      function(function),
      builder(scope.get_context().get_arena(), function) {}

auto Library::Compiler::Function::compile(
    Cursor& cursor,
    Scope& scope,
    const Ttx::Function& function,
    Range source) -> const Execution::Body* {
  if (source.is_empty()) {
    return nullptr;
  }

  Count return_index = cursor.get_token_index();
  cursor.seek_token(source.start);
  Library::Compiler::Function compiler(cursor, scope, function);
  const Execution::Body* body = compiler.build();
  Bool consumed = cursor.get_token_index() == source.get_end();
  cursor.seek_token(return_index);
  return consumed ? body : nullptr;
}

auto Library::Compiler::Function::publish(
    Cursor& cursor,
    Scope& scope,
    const Ttx::Type& owner,
    View::Vector<Range> sources,
    View::Vector<Base::Definition> definitions) -> Bool {
  View::Vector<Ttx::Function> functions = owner.get_functions();
  if (sources.get_size() != functions.get_size() ||
      definitions.get_size() != functions.get_size()) {
    return False;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    Linkage linkage = Linkage::internal(
        scope.get_context().get_arena(), owner, functions[i],
        scope.get_context().get_module());
    if (!linkage.is_valid() || !scope.get_context().define_linkage(linkage)) {
      return False;
    }
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (sources[i].is_empty()) {
      if (!scope.get_context().define_implementation(
              functions[i], definitions[i])) {
        return False;
      }

      continue;
    }

    const Execution::Body* body =
        compile(cursor, scope, functions[i], sources[i]);
    if (body == nullptr || !scope.get_context().define_implementation(
                               functions[i], *body, definitions[i])) {
      return False;
    }
  }

  return True;
}

auto Library::Compiler::Function::resolve_type(
    const Base::Expression::Value& value) const -> const Ttx::Type* {
  if (value.get_kind() != Base::Expression::Value::Kind::Reference) {
    return nullptr;
  }

  View::Vector<Token> tokens = value.get_tokens();
  if (tokens.is_empty()) {
    return nullptr;
  }

  const Ttx::Type* type = scope.find_type(tokens[0].get_text());
  for (Count i = 1; type != nullptr && i < tokens.get_size(); i += 2) {
    if (i + 1 >= tokens.get_size() ||
        tokens[i].get_class() != Class::Type::TypeAccessOp ||
        tokens[i + 1].get_class() != Class::Type::Type) {
      return nullptr;
    }

    type = type->find_type(tokens[i + 1].get_text());
  }

  return type;
}

auto Library::Compiler::Function::emit_call(
    const Ttx::Function& target,
    const Linkage& linkage,
    const Base::Expression::Pack& source_arguments) -> Range {
  View::Vector<Base::Expression::Value> values = source_arguments.get_values();
  View::Vector<Ttx::Member> parameters = target.get_parameters().get_members();
  if (values.get_size() != parameters.get_size()) {
    return {Count(-1), 0};
  }

  Managed::Vector<Execution::Operand> arguments(
      scope.get_context().get_arena());
  for (Count i = 0; i < values.get_size(); i++) {
    auto operand = lower(values[i], parameters[i].get_type());
    if (operand.is_null()) {
      return {Count(-1), 0};
    }

    arguments.insert(operand);
  }

  return builder.call(linkage, arguments.get_view());
}

auto Library::Compiler::Function::literal_fits(
    const Base::Expression::Value& value,
    const Ttx::Type& expected) const -> Bool {
  auto fits = [&](View::Bytes name) {
    const Ttx::Type* type = Standard::Types::find_type(name);
    return type != nullptr && expected.equivalent_to(*type);
  };
  switch (value.get_kind()) {
  case Base::Expression::Value::Kind::String:
  case Base::Expression::Value::Kind::Bytes:
    return fits("View[Bytes]"_view);
  case Base::Expression::Value::Kind::Boolean:
    return fits("Bool"_view);
  case Base::Expression::Value::Kind::Numeric: {
    View::Bytes name = expected.canonical().get_name();
    Bool signed_integer = name == "Signed_8"_view || name == "Signed_16"_view ||
                          name == "Signed_32"_view || name == "Signed_64"_view;
    Bool unsigned_integer = name == "Bits_8"_view || name == "Bits_16"_view ||
                            name == "Bits_32"_view || name == "Bits_64"_view;
    return signed_integer || (!value.is_negative() && unsigned_integer);
  }
  case Base::Expression::Value::Kind::Real:
    return fits("Real_64"_view) || fits("Real_32"_view);
  default:
    return False;
  }
}

auto Library::Compiler::Function::infer_type(
    const Base::Expression::Value& value) const -> const Ttx::Type* {
  switch (value.get_kind()) {
  case Base::Expression::Value::Kind::String:
  case Base::Expression::Value::Kind::Bytes:
    return Standard::Types::find_type("View[Bytes]"_view);
  case Base::Expression::Value::Kind::Boolean:
    return Standard::Types::find_type("Bool"_view);
  case Base::Expression::Value::Kind::Numeric:
    return Standard::Types::find_type(
        value.is_negative() ? "Signed_64"_view : "Bits_64"_view);
  case Base::Expression::Value::Kind::Real:
    return Standard::Types::find_type("Real_64"_view);
  case Base::Expression::Value::Kind::Reference: {
    View::Vector<Token> tokens = value.get_tokens();
    if (tokens.get_size() == 1) {
      const Ttx::Member* parameter =
          function.get_parameters().find_member(value.get_value());
      return parameter == nullptr ? nullptr : &parameter->get_type();
    }

    return nullptr;
  }
  case Base::Expression::Value::Kind::Call: {
    const Base::Expression::Value* source_owner = value.get_call_owner();
    const Ttx::Type* owner =
        source_owner == nullptr ? nullptr : resolve_type(*source_owner);
    const Ttx::Function* target =
        owner == nullptr ? nullptr
                         : owner->find_function(value.get_call_name());
    return target == nullptr || target->get_result().get_member_count() != 1
               ? nullptr
               : &target->get_result().member_at(0).get_type();
  }
  case Base::Expression::Value::Kind::Binary:
    return value.get_operator() == Base::Expression::Value::Operator::Equal
               ? Standard::Types::find_type("Bool"_view)
           : value.get_left() == nullptr ? nullptr
                                         : infer_type(*value.get_left());
  default:
    return nullptr;
  }
}

auto Library::Compiler::Function::lower_reference(
    const Base::Expression::Value& value,
    const Ttx::Type& expected) -> Execution::Operand {
  View::Vector<Token> tokens = value.get_tokens();
  if (tokens.get_size() == 1) {
    const Ttx::Member* parameter =
        function.get_parameters().find_member(value.get_value());
    if (parameter == nullptr) {
      report_missing_parameter(value);
      return Execution::Operand();
    }

    if (!parameter->get_type().equivalent_to(expected)) {
      Managed::Bytes message(scope.get_context().get_arena());
      message.concat("Parameter `"_view);
      message.concat(value.get_value());
      message.concat("` does not fit the expected type."_view);
      cursor.range_error(tokens[0], tokens[0], message);
      return Execution::Operand();
    }

    return builder.parameter(*parameter);
  }

  if (tokens.get_size() < 3 ||
      tokens[tokens.get_size() - 2].get_class() != Class::Type::AddressOp ||
      tokens[tokens.get_size() - 1].get_class() != Class::Type::Addressable) {
    return Execution::Operand();
  }

  const Ttx::Type* owner = scope.find_type(tokens[0].get_text());
  for (Count i = 1; owner != nullptr && i + 2 < tokens.get_size(); i += 2) {
    if (tokens[i].get_class() != Class::Type::TypeAccessOp ||
        tokens[i + 1].get_class() != Class::Type::Type) {
      return Execution::Operand();
    }

    owner = owner->find_type(tokens[i + 1].get_text());
  }

  const Ttx::Member* member =
      owner == nullptr
          ? nullptr
          : owner->find_member(tokens[tokens.get_size() - 1].get_text());
  const Base::Definition* definition =
      member == nullptr ? nullptr
                        : scope.get_context().find_definition(*member);
  if (member == nullptr || definition == nullptr ||
      definition->get_initializer() == nullptr ||
      !owner->equivalent_to(expected)) {
    return Execution::Operand();
  }

  return lower(*definition->get_initializer(), member->get_type());
}

auto Library::Compiler::Function::report_missing_parameter(
    const Base::Expression::Value& value) -> Bool {
  View::Vector<Token> tokens = value.get_tokens();
  if (tokens.get_size() != 1 ||
      function.get_parameters().find_member(value.get_value()) != nullptr) {
    return False;
  }

  Managed::Bytes message(scope.get_context().get_arena());
  message.concat("Function has no parameter named `"_view);
  message.concat(value.get_value());
  message.concat("`."_view);

  View::Bytes hint = Diagnostics::Suggestions::possible_candidate(
      scope.get_context().get_arena(), value.get_value(),
      function.get_parameters());
  cursor.range_error(tokens[0], tokens[0], message, hint);
  return True;
}

auto Library::Compiler::Function::lower_call(
    const Base::Expression::Value& value) -> Execution::Operand {
  Range results = emit_call(value);
  if (results.start == Count(-1) || results.size != 1) {
    return Execution::Operand();
  }

  return Execution::Operand(Execution::Addressable(results.start));
}

auto Library::Compiler::Function::emit_call(
    const Base::Expression::Value& value) -> Range {
  const Base::Expression::Value* source_owner = value.get_call_owner();
  const Base::Expression::Pack* arguments = value.get_call_arguments();
  if (source_owner == nullptr || arguments == nullptr) {
    return {Count(-1), 0};
  }

  const Ttx::Type* owner = resolve_type(*source_owner);
  const Ttx::Function* target =
      owner == nullptr ? nullptr : owner->find_function(value.get_call_name());
  if (target == nullptr) {
    return {Count(-1), 0};
  }

  const Linkage* linkage = scope.get_context().find_linkage(*target);
  if (linkage == nullptr || linkage->get_symbol().is_empty()) {
    return {Count(-1), 0};
  }

  return emit_call(*target, *linkage, *arguments);
}

auto Library::Compiler::Function::lower(
    const Base::Expression::Value& value,
    const Ttx::Type& expected) -> Execution::Operand {
  switch (value.get_kind()) {
  case Base::Expression::Value::Kind::String:
  case Base::Expression::Value::Kind::Bytes:
    return literal_fits(value, expected)
               ? builder.constant(Execution::Constant(value.get_value()))
               : Execution::Operand();
  case Base::Expression::Value::Kind::Boolean:
    return literal_fits(value, expected)
               ? builder.constant(Execution::Constant(value.get_flag()))
               : Execution::Operand();
  case Base::Expression::Value::Kind::Numeric: {
    Reader::Textual reader(value.get_value());
    Bits_64 magnitude = reader.read_unsigned();
    if (!literal_fits(value, expected)) {
      return Execution::Operand();
    }

    const Ttx::Type* signed_type = Standard::Types::find_type("Signed_64"_view);
    if (signed_type != nullptr && expected.equivalent_to(*signed_type)) {
      constexpr Bits_64 signed_limit = Bits_64(1) << 63;
      if (magnitude > signed_limit ||
          (!value.is_negative() && magnitude == signed_limit)) {
        return Execution::Operand();
      }

      Signed_64 number = magnitude == signed_limit
                             ? (-9223372036854775807LL - 1)
                             : Signed_64(magnitude);
      if (value.is_negative() && magnitude != signed_limit) {
        number = -number;
      }

      return builder.constant(Execution::Constant(number));
    }

    return builder.constant(Execution::Constant(magnitude));
  }
  case Base::Expression::Value::Kind::Real: {
    Reader::Textual reader(value.get_value());
    Real_64 number = reader.read_real_64();
    return !literal_fits(value, expected)
               ? Execution::Operand()
               : builder.constant(
                     Execution::Constant(
                         value.is_negative() ? -number : number));
  }
  case Base::Expression::Value::Kind::Reference:
    return lower_reference(value, expected);
  case Base::Expression::Value::Kind::Call:
    return lower_call(value);
  case Base::Expression::Value::Kind::Binary: {
    if (value.get_left() == nullptr || value.get_right() == nullptr) {
      return Execution::Operand();
    }

    const Ttx::Type* operand_type = infer_type(*value.get_left());
    if (operand_type == nullptr) {
      operand_type = infer_type(*value.get_right());
    }

    const Ttx::Type* result_type =
        value.get_operator() == Base::Expression::Value::Operator::Equal
            ? Standard::Types::find_type("Bool"_view)
            : operand_type;
    if (operand_type == nullptr || result_type == nullptr ||
        !result_type->equivalent_to(expected)) {
      return Execution::Operand();
    }

    Execution::Operand left = lower(*value.get_left(), *operand_type);
    Execution::Operand right = lower(*value.get_right(), *operand_type);
    if (left.is_null() || right.is_null()) {
      return Execution::Operand();
    }

    Execution::Binary::Operator op;
    switch (value.get_operator()) {
    case Base::Expression::Value::Operator::Add:
      op = Execution::Binary::Operator::Add;
      break;
    case Base::Expression::Value::Operator::Subtract:
      op = Execution::Binary::Operator::Subtract;
      break;
    case Base::Expression::Value::Operator::Multiply:
      op = Execution::Binary::Operator::Multiply;
      break;
    case Base::Expression::Value::Operator::Divide:
      op = Execution::Binary::Operator::Divide;
      break;
    case Base::Expression::Value::Operator::Remainder:
      op = Execution::Binary::Operator::Remainder;
      break;
    case Base::Expression::Value::Operator::Equal:
      op = Execution::Binary::Operator::Equal;
      break;
    default:
      return Execution::Operand();
    }

    return builder.binary(op, *operand_type, expected, left, right);
  }
  default:
    return Execution::Operand();
  }
}

auto Library::Compiler::Function::evaluate_return() -> Bool {
  cursor.consume();
  Managed::Vector<Execution::Operand> values(scope.get_context().get_arena());
  Ttx::Layout result = function.get_result();
  if (!cursor.matches(Class::Type::EndStatement)) {
    Base::Expression::Value value =
        Base::Expression::Value::evaluate(cursor, scope.get_context());
    if (value.is_empty()) {
      return False;
    }

    const Base::Expression::Pack* pack = value.get_pack();
    if (pack == nullptr) {
      if (result.get_member_count() != 1) {
        cursor.token_error(
            "Return value does not match the result layout."_view);
        return False;
      }

      Count error_count = cursor.get_errors().get_size();
      auto operand = lower(value, result.member_at(0).get_type());
      if (operand.is_null()) {
        if (cursor.get_errors().get_size() == error_count) {
          cursor.token_error("Return value could not be compiled."_view);
        }

        return False;
      }

      values.insert(operand);
    } else {
      View::Vector<Base::Expression::Value> packed_values = pack->get_values();
      Managed::Vector<Ttx::Member> value_schema(
          scope.get_context().get_arena());
      for (Count i = 0; i < packed_values.get_size(); i++) {
        const Ttx::Type* type = infer_type(packed_values[i]);
        if (type == nullptr) {
          Count error_count = cursor.get_errors().get_size();
          if (packed_values[i].get_kind() ==
              Base::Expression::Value::Kind::Reference) {
            report_missing_parameter(packed_values[i]);
          }

          if (cursor.get_errors().get_size() == error_count) {
            cursor.token_error(
                "Return pack value type could not be resolved."_view);
          }

          return False;
        }

        value_schema.insert(Ttx::Member(View::Bytes(), *type));
      }

      Ttx::Layout source = pack->schema(
          scope.get_context().get_arena(), value_schema.get_view());
      if (!source.fits(result)) {
        cursor.token_error(
            "Return pack does not fit the function result."_view);
        return False;
      }

      for (Count i = 0; i < result.get_member_count(); i++) {
        Count source_index = source.source_index_for(result, i);
        if (source_index == Count(-1)) {
          cursor.token_error(
              "Defaulted return members cannot be compiled yet."_view);
          return False;
        }

        Count error_count = cursor.get_errors().get_size();
        auto operand =
            lower(packed_values[source_index], result.member_at(i).get_type());
        if (operand.is_null()) {
          if (cursor.get_errors().get_size() == error_count) {
            cursor.token_error("Return value could not be compiled."_view);
          }

          return False;
        }

        values.insert(operand);
      }
    }
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after return."_view)) {
    return False;
  }

  if (!builder.return_values(values.get_view())) {
    cursor.token_error("Return values do not fit the function result."_view);
    return False;
  }

  return True;
}

auto Library::Compiler::Function::evaluate_call() -> Bool {
  const Token owner_token = cursor.current();
  const Ttx::Type* owner = scope.resolve_type(cursor);
  if (owner == nullptr) {
    cursor.range_error(
        owner_token, owner_token,
        "Library call target could not be resolved."_view);
    return False;
  }

  if (!cursor.require(
          Class::Type::CallOp, "Expected `->` in library call."_view)) {
    return False;
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected library function name."_view);
  if (name == nullptr) {
    return False;
  }

  const Ttx::Function* target = owner->find_function(name->get_text());
  if (target == nullptr) {
    cursor.range_error(
        *name, *name, "Library call function could not be resolved."_view);
    return False;
  }

  const Base::Expression::Pack* arguments =
      Base::Expression::Pack::evaluate(cursor, scope.get_context());
  const Linkage* linkage = scope.get_context().find_linkage(*target);
  if (arguments == nullptr || linkage == nullptr ||
      linkage->get_symbol().is_empty()) {
    cursor.range_error(
        owner_token, *name, "Library call target has no linkage symbol."_view);
    return False;
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after library call."_view)) {
    return False;
  }

  Range results = emit_call(*target, *linkage, *arguments);
  if (results.start == Count(-1)) {
    cursor.range_error(
        owner_token, *name, "Library call could not be compiled."_view);
    return False;
  }

  return True;
}

auto Library::Compiler::Function::build() -> const Execution::Body* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library function signature."_view)) {
    return nullptr;
  }

  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeEnd)) {
      cursor.consume();
      const auto* body = builder.finish();
      if (body == nullptr) {
        cursor.token_error("Library function body is incomplete."_view);
      }

      return body;
    }

    if (cursor.is_one_of({{Class::Type::Comment, Class::Type::Disabled}})) {
      cursor.consume();
      continue;
    }

    if (builder.is_terminated()) {
      cursor.token_error("Library operation follows a return."_view);
      return nullptr;
    }

    if (cursor.matches(Class::Type::Return)) {
      if (!evaluate_return()) {
        return nullptr;
      }

      continue;
    }

    if (cursor.matches(Class::Type::Type)) {
      if (!evaluate_call()) {
        return nullptr;
      }

      continue;
    }

    cursor.token_error("Expected Library execution operation."_view);
    return nullptr;
  }

  cursor.token_error("Expected `}` after library function body."_view);
  return nullptr;
}
