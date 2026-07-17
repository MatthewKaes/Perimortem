// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/compiler/function.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
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
    View::Bytes owner_path,
    View::Vector<Ttx::Function> functions,
    View::Vector<Range> sources,
    View::Vector<Base::Definition> definitions,
    Bool addressable) -> Bool {
  if (sources.get_size() != functions.get_size() ||
      definitions.get_size() != functions.get_size()) {
    return False;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    Abi::Linkage linkage =
        addressable ? Abi::Linkage::addressable(
                          scope.get_context().get_arena(), owner, functions[i],
                          scope.get_context().get_unit_name(),
                          scope.get_context().get_module(), owner_path)
                    : Abi::Linkage::type(
                          scope.get_context().get_arena(), owner, functions[i],
                          scope.get_context().get_unit_name(),
                          scope.get_context().get_module(), owner_path);
    Bool defined = scope.get_context().define_linkage(linkage);
    if (!defined) {
      return False;
    }
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (sources[i].is_empty()) {
      Bool defined = scope.get_context().define_implementation(
          functions[i], definitions[i]);
      if (!defined) {
        return False;
      }

      continue;
    }

    const Execution::Body* body =
        compile(cursor, scope, functions[i], sources[i]);
    if (body == nullptr) {
      return False;
    }

    Bool defined = scope.get_context().define_implementation(
        functions[i], *body, definitions[i]);
    if (!defined) {
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
    const Abi::Linkage& linkage,
    const Base::Expression::Pack& source_arguments,
    View::Vector<Execution::Operand> leading) -> Range {
  View::Vector<Base::Expression::Value> values = source_arguments.get_values();
  View::Vector<Ttx::Member> parameters = target.get_parameters().get_members();
  if (leading.get_size() + values.get_size() != parameters.get_size()) {
    return {Count(-1), 0};
  }

  Managed::Vector<Execution::Operand> arguments(
      scope.get_context().get_arena());
  for (Count i = 0; i < leading.get_size(); i++) {
    if (leading[i].is_null()) {
      return {Count(-1), 0};
    }

    arguments.insert(leading[i]);
  }

  for (Count i = 0; i < values.get_size(); i++) {
    auto operand =
        lower(values[i], parameters[leading.get_size() + i].get_type());
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
    Bool unsigned_integer =
        name == "Unsigned_8"_view || name == "Unsigned_16"_view ||
        name == "Unsigned_32"_view || name == "Unsigned_64"_view;
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
        value.is_negative() ? "Signed_64"_view : "Unsigned_64"_view);
  case Base::Expression::Value::Kind::Real:
    return Standard::Types::find_type("Real_64"_view);
  case Base::Expression::Value::Kind::Type:
    return Standard::Types::find_type("Type"_view);
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
    const Base::Expression::Value& source_owner = value.get_call_owner();
    const Ttx::Type* owner = resolve_type(source_owner);
    const Ttx::Function* target =
        owner == nullptr ? nullptr
                         : owner->find_type_function(value.get_call_name());
    if (target == nullptr) {
      owner = infer_type(source_owner);
      target = owner == nullptr
                   ? nullptr
                   : owner->find_addressable_function(value.get_call_name());
    }

    return target == nullptr || target->get_result().get_member_count() != 1
               ? nullptr
               : &target->get_result().member_at(0).get_type();
  }
  case Base::Expression::Value::Kind::Binary:
    return value.get_operator() == Base::Expression::Value::Operator::Equal
               ? Standard::Types::find_type("Bool"_view)
               : infer_type(value.get_left());
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
      !definition->has_initializer() || !owner->equivalent_to(expected)) {
    return Execution::Operand();
  }

  return lower(definition->get_initializer(), member->get_type());
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
  const Base::Expression::Value& source_owner = value.get_call_owner();
  const Base::Expression::Pack& arguments = value.get_call_arguments();
  const Ttx::Type* owner = resolve_type(source_owner);
  const Ttx::Function* target =
      owner == nullptr ? nullptr
                       : owner->find_type_function(value.get_call_name());
  if (target != nullptr) {
    const Abi::Linkage* linkage = scope.get_context().find_linkage(*target);
    return linkage == nullptr || linkage->get_symbol().is_empty()
               ? Range{Count(-1), 0}
               : emit_call(*target, *linkage, arguments);
  }

  owner = infer_type(source_owner);
  target = owner == nullptr
               ? nullptr
               : owner->find_addressable_function(value.get_call_name());
  if (target == nullptr || target->get_parameters().is_empty()) {
    return {Count(-1), 0};
  }

  Execution::Operand receiver =
      lower(source_owner, target->get_parameters().member_at(0).get_type());
  if (receiver.is_null()) {
    return {Count(-1), 0};
  }

  const Abi::Linkage* linkage = scope.get_context().find_linkage(*target);
  if (linkage == nullptr || linkage->get_symbol().is_empty()) {
    return {Count(-1), 0};
  }

  Static::Vector<Execution::Operand, 1> leading = {{receiver}};
  return emit_call(*target, *linkage, arguments, leading);
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
    Unsigned_64 magnitude = reader.read_unsigned();
    if (!literal_fits(value, expected)) {
      return Execution::Operand();
    }

    const Ttx::Type* signed_type = Standard::Types::find_type("Signed_64"_view);
    if (signed_type != nullptr && expected.equivalent_to(*signed_type)) {
      constexpr Unsigned_64 signed_limit = Unsigned_64(1) << 63;
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
    const Ttx::Type* operand_type = infer_type(value.get_left());
    if (operand_type == nullptr) {
      operand_type = infer_type(value.get_right());
    }

    const Ttx::Type* result_type =
        value.get_operator() == Base::Expression::Value::Operator::Equal
            ? Standard::Types::find_type("Bool"_view)
            : operand_type;
    if (operand_type == nullptr || result_type == nullptr ||
        !result_type->equivalent_to(expected)) {
      return Execution::Operand();
    }

    Execution::Operand left = lower(value.get_left(), *operand_type);
    Execution::Operand right = lower(value.get_right(), *operand_type);
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

    if (value.get_kind() != Base::Expression::Value::Kind::Pack) {
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
      const Base::Expression::Pack& pack = value.get_pack();
      View::Vector<Base::Expression::Value> packed_values = pack.get_values();
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

      Ttx::Layout source =
          pack.schema(scope.get_context().get_arena(), value_schema.get_view());
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

  Bool has_statement_end = cursor.require(
      Class::Type::EndStatement, "Expected `;` after return."_view);
  if (!has_statement_end) {
    return False;
  }

  Bool returned = builder.return_values(values.get_view());
  if (!returned) {
    cursor.token_error("Return values do not fit the function result."_view);
    return False;
  }

  return True;
}

auto Library::Compiler::Function::evaluate_call() -> Bool {
  const Token& owner_token = cursor.current();
  Base::Expression::Value value =
      Base::Expression::Value::evaluate(cursor, scope.get_context());
  if (value.get_kind() != Base::Expression::Value::Kind::Call) {
    cursor.range_error(
        owner_token, owner_token, "Expected Library call expression."_view);
    return False;
  }

  Bool has_statement_end = cursor.require(
      Class::Type::EndStatement, "Expected `;` after library call."_view);
  if (!has_statement_end) {
    return False;
  }

  Range results = emit_call(value);
  if (results.start == Count(-1)) {
    cursor.range_error(
        owner_token, owner_token, "Library call could not be compiled."_view);
    return False;
  }

  return True;
}

auto Library::Compiler::Function::build() -> const Execution::Body* {
  Bool has_scope = cursor.require(
      Class::Type::ScopeStart,
      "Expected `{` after library function signature."_view);
  if (!has_scope) {
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
      Bool evaluated = evaluate_return();
      if (!evaluated) {
        return nullptr;
      }

      continue;
    }

    if (cursor.is_one_of(
            {{Class::Type::Type, Class::Type::Addressable,
              Class::Type::Self}})) {
      Bool evaluated = evaluate_call();
      if (!evaluated) {
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
