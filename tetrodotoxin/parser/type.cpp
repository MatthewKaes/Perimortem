// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/type.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/parser/builtins.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/generics.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

namespace Tetrodotoxin::Parser {

using Generic = Ttx::Model::Types::Generic;
using Argument = Generic::Argument;

// Nested Generic arguments recurse through the same progressive Type parser,
// so this file-local declaration closes that implementation cycle without
// exposing parser machinery in the public header.
static auto parse_value(Cursor& cursor, const Abstract& context)
    -> Option<const Ttx::Model::Type&>;

static auto unresolved_segment_error(Cursor& cursor, Token segment) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Type segment `"_view
         << segment.caculate_text(cursor.get_source_text())
         << "` does not resolve in the selected Abstract context."_view;
  cursor.create_token_error(segment, message);
}

static auto wrong_contract_error(
    Cursor& cursor,
    Token start,
    Token end,
    const Abstract& selected) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Type reference resolves to `"_view << selected.get_name()
         << "`, which does not prove the Type contract."_view;
  cursor.create_expression_error(start, end, message);
}

static constexpr auto parameter_name(Generic::Parameters parameter)
    -> View::Bytes {
  switch (parameter) {
  case Generic::Parameters::Type:
    return "Type"_view;
  case Generic::Parameters::Unsigned_64:
    return "Unsigned_64"_view;
  case Generic::Parameters::Signed_64:
    return "Signed_64"_view;
  case Generic::Parameters::Bool:
    return "Bool"_view;
  }

  __builtin_unreachable();
}

static constexpr auto parse_magnitude(
    View::Bytes text,
    Unsigned_64 radix,
    Unsigned_64 maximum,
    Unsigned_64& value) -> Bool {
  Count index = radix == 16 ? Count(2) : Count(0);
  Unsigned_64 parsed = 0;
  if (index == text.get_size()) {
    return False;
  }

  for (; index < text.get_size(); index++) {
    Unsigned_8 character = text[index];
    Unsigned_64 digit = 0;
    if (character >= '0' && character <= '9') {
      digit = character - '0';
    } else if (character >= 'a' && character <= 'f') {
      digit = character - 'a' + 10;
    } else if (character >= 'A' && character <= 'F') {
      digit = character - 'A' + 10;
    } else {
      return False;
    }

    if (digit >= radix || parsed > (maximum - digit) / radix) {
      return False;
    }

    parsed = parsed * radix + digit;
  }

  value = parsed;
  return True;
}

static auto missing_arguments_error(
    Cursor& cursor,
    const Generic& generic,
    View::Vector<Generic::Parameters> parameters,
    Count first_missing) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Not enough arguments for generic Type `"_view << generic.get_name()
         << "`: expected "_view << parameters.get_size() << ", received "_view
         << first_missing << ". Missing "_view;
  for (Count i = first_missing; i < parameters.get_size(); i++) {
    if (i != first_missing) {
      output << ", "_view;
    }

    output << parameter_name(parameters[i]);
  }

  output << "."_view;
  cursor.create_token_error(message);
}

static auto unexpected_argument_error(
    Cursor& cursor,
    const Generic& generic,
    Generic::Parameters expected) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Unexpected argument for generic Type `"_view << generic.get_name()
         << "`; expected "_view << parameter_name(expected) << "."_view;
  cursor.create_token_error(message);
}

static auto parse_argument(
    Cursor& cursor,
    const Abstract& context,
    const Generic& generic,
    Generic::Parameters parameter,
    Managed::Vector<Argument>& arguments) -> Bool {
  if (parameter == Generic::Parameters::Type) {
    if (!cursor.matches(Code::Type::Type)) {
      unexpected_argument_error(cursor, generic, parameter);
      return False;
    }

    Option<const Ttx::Model::Type&> selected = parse_value(cursor, context);
    return selected.visit(
        [](const None&) { return False; },
        [&arguments](const Ttx::Model::Type& type) {
          Argument argument(type);
          arguments.insert(argument);
          return True;
        });
  }

  if (parameter == Generic::Parameters::Unsigned_64) {
    const Bool numeric =
        cursor.matches(Code::Type::Numeric) || cursor.matches(Code::Type::Hex);
    if (!numeric) {
      unexpected_argument_error(cursor, generic, parameter);
      return False;
    }

    Token token = cursor.consume();
    View::Bytes text = token.caculate_text(cursor.get_source_text());
    Unsigned_64 value = 0;
    Unsigned_64 radix = token.get_code() == Code::Type::Hex ? 16 : 10;
    Bool valid = parse_magnitude(text, radix, Unsigned_64(-1), value);
    if (!valid) {
      Managed::Bytes message(cursor.get_arena());
      Stream::Textual<Managed::Bytes> output(message);
      output << "Unsigned argument for generic Type `"_view
             << generic.get_name() << "` is outside Unsigned_64."_view;
      cursor.create_token_error(token, message);
      return False;
    }

    Argument argument(value);
    arguments.insert(argument);
    return True;
  }

  if (parameter == Generic::Parameters::Signed_64) {
    Bool negative = cursor.matches(Code::Type::SubOp);
    if (negative) {
      cursor.consume();
    }

    const Bool numeric =
        cursor.matches(Code::Type::Numeric) || cursor.matches(Code::Type::Hex);
    if (!numeric) {
      unexpected_argument_error(cursor, generic, parameter);
      return False;
    }

    Token token = cursor.consume();
    View::Bytes text = token.caculate_text(cursor.get_source_text());
    Unsigned_64 radix = token.get_code() == Code::Type::Hex ? 16 : 10;
    Unsigned_64 maximum = Unsigned_64(-1) >> 1;
    if (negative) {
      maximum++;
    }

    Unsigned_64 magnitude = 0;
    Bool valid = parse_magnitude(text, radix, maximum, magnitude);
    if (!valid) {
      Managed::Bytes message(cursor.get_arena());
      Stream::Textual<Managed::Bytes> output(message);
      output << "Signed argument for generic Type `"_view << generic.get_name()
             << "` is outside Signed_64."_view;
      cursor.create_token_error(token, message);
      return False;
    }

    Signed_64 value = 0;
    if (negative) {
      constexpr Signed_64 minimum = -9223372036854775807LL - 1;
      value = magnitude == maximum ? minimum : -Signed_64(magnitude);
    } else {
      value = Signed_64(magnitude);
    }

    Argument argument(value);
    arguments.insert(argument);
    return True;
  }

  if (cursor.matches(Code::Type::True)) {
    cursor.consume();
    Argument argument(True);
    arguments.insert(argument);
    return True;
  }
  if (cursor.matches(Code::Type::False)) {
    cursor.consume();
    Argument argument(False);
    arguments.insert(argument);
    return True;
  }

  unexpected_argument_error(cursor, generic, parameter);
  return False;
}

static auto parse_generic(
    Cursor& cursor,
    const Abstract& context,
    const Generic& generic,
    Token start) -> Option<const Ttx::Model::Type&> {
  cursor.consume();
  View::Vector<Generic::Parameters> parameters = generic.get_parameterization();
  Managed::Vector<Argument> arguments(cursor.get_arena());
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (cursor.matches(Code::Type::LayoutEnd)) {
      missing_arguments_error(cursor, generic, parameters, i);
      return none;
    }

    // The parser validates each declared parameter kind before the formula is
    // asked to find a materialized Type.
    Bool parsed =
        parse_argument(cursor, context, generic, parameters[i], arguments);
    if (!parsed) {
      return none;
    }

    if (i + 1 == parameters.get_size()) {
      continue;
    }
    if (cursor.matches(Code::Type::LayoutEnd)) {
      missing_arguments_error(cursor, generic, parameters, i + 1);
      return none;
    }
    if (!cursor.matches(Code::Type::PackingOp)) {
      Managed::Bytes message(cursor.get_arena());
      Stream::Textual<Managed::Bytes> output(message);
      output << "Expected `,` between arguments for generic Type `"_view
             << generic.get_name() << "`."_view;
      cursor.create_token_error(message);
      return none;
    }

    cursor.consume();
  }

  if (cursor.matches(Code::Type::PackingOp)) {
    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Too many arguments for generic Type `"_view << generic.get_name()
           << "`: expected "_view << parameters.get_size() << "."_view;
    cursor.create_token_error(
        message, "Remove arguments after the declared parameterization."_view);
    return none;
  }

  Managed::Bytes closing_message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> closing_output(closing_message);
  closing_output << "Expected `]` after arguments for generic Type `"_view
                 << generic.get_name() << "`."_view;
  Token end = cursor.require(Code::Type::LayoutEnd, closing_message);
  if (!end.is_valid()) {
    return none;
  }

  Option<Ttx::Model::Type&> materialized = generic.find(arguments.get_view());
  return materialized.visit(
      [&](const None&) -> Option<const Ttx::Model::Type&> {
        Managed::Bytes message(cursor.get_arena());
        Stream::Textual<Managed::Bytes> output(message);
        output << "Generic Type `"_view << generic.get_name()
               << "` rejected its parsed arguments."_view;
        cursor.create_expression_error(start, end, message);
        return none;
      },
      [](Ttx::Model::Type& type) -> Option<const Ttx::Model::Type&> {
        return type;
      });
}

static auto parse_value(Cursor& cursor, const Abstract& context)
    -> Option<const Ttx::Model::Type&> {
  if (!cursor.matches(Code::Type::Type)) {
    Token found = cursor.consume();
    cursor.create_token_error(
        found, "Expected a Type reference."_view,
        "Type names begin with an uppercase ASCII letter."_view);
    return none;
  }

  Token start = cursor.current();
  Token end = cursor.consume();
  View::Bytes name = end.caculate_text(cursor.get_source_text());
  Option<const Ttx::Model::Type&> builtin = Builtins::find(name);
  const Abstract& first = builtin.visit(
      [&context, name](const None&) -> const Abstract& {
        return context.resolve_context(name);
      },
      [](const Ttx::Model::Type& type) -> const Abstract& { return type; });
  Reference<Abstract> selected(first);
  if (selected.get().is<Invalid>()) {
    unresolved_segment_error(cursor, end);
    return none;
  }

  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    Token nested = cursor.require(
        Code::Type::Type,
        "Expected a Type name after the `::` access operator."_view);
    if (!nested.is_valid()) {
      return none;
    }

    // require() consumes the nested segment. The selected Abstract alone owns
    // its next lookup, so a failed nested route cannot fall back to the root.
    View::Bytes nested_name = nested.caculate_text(cursor.get_source_text());
    const Abstract& nested_result = selected.get().resolve_context(nested_name);
    if (nested_result.is<Invalid>()) {
      unresolved_segment_error(cursor, nested);
      return none;
    }

    selected = Reference<Abstract>(nested_result);
    end = nested;
  }

  const Abstract& resolved = selected.get().resolve();
  if (cursor.matches(Code::Type::LayoutStart)) {
    if (!resolved.is<Generic>()) {
      cursor.create_token_error(
          "Type arguments require a Generic Type formula."_view,
          "Remove the arguments or select a Generic such as View, Access, or Fixed."_view);
      return none;
    }

    return parse_generic(cursor, context, resolved.assume<Generic>(), start);
  }

  if (resolved.is<Generic>()) {
    View::Vector<Generic::Parameters> parameters =
        resolved.assume<Generic>().get_parameterization();
    missing_arguments_error(cursor, resolved.assume<Generic>(), parameters, 0);
    return none;
  }

  if (!resolved.is<Ttx::Model::Type>()) {
    wrong_contract_error(cursor, start, end, resolved);
    return none;
  }

  return resolved.assume<Ttx::Model::Type>();
}

auto Type::parse(Cursor& cursor, const Abstract& context)
    -> Option<const Ttx::Model::Type&> {
  Option<const Ttx::Model::Type&> parsed = parse_value(cursor, context);
  Bool failed = parsed.visit(
      [](const None&) { return True; },
      [](const Ttx::Model::Type&) { return False; });
  if (failed) {
    cursor.recover_to_statement();
  }

  return parsed;
}

}  // namespace Tetrodotoxin::Parser
