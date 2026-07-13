// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/expression/type.hpp"

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto consume_type_arguments(Cursor& cursor) -> Bool {
  if (!cursor.matches(Class::Type::IndexStart)) {
    return True;
  }

  Count depth = 0;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::IndexStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexEnd)) {
      cursor.consume();
      depth--;
      if (depth == 0) {
        return True;
      }

      continue;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `]` after type arguments."_view);
  return False;
}

static auto type_argument(Cursor& cursor, Base::Context& context)
    -> const Ttx::Type* {
  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* argument = Base::Expression::Type::evaluate(cursor, context);
  if (argument == nullptr && cursor.get_errors().get_size() == error_count) {
    cursor.token_error("Type argument could not be resolved."_view);
  }

  return argument;
}

static auto parameterized_name(
    Base::Context& context,
    const Ttx::Type& root,
    const Ttx::Type& argument,
    View::Bytes extent = View::Bytes()) -> Managed::Bytes {
  Managed::Bytes name(context.get_arena(), root.get_name());
  name.concat("["_view);
  name.concat(argument.get_name());
  if (!extent.is_empty()) {
    name.concat(","_view);
    name.concat(extent);
  }

  name.concat("]"_view);
  return name;
}

static auto parameterize(
    Base::Context& context,
    const Ttx::Type& root,
    const Ttx::Type& argument,
    Count extent,
    View::Bytes name) -> const Ttx::Type* {
  const Ttx::Member member(""_view, argument);
  return &context.parameterize_type(
      root, Ttx::Layout(View::Vector<Ttx::Member>(&member, 1)), extent, name);
}

static auto evaluate_view_arguments(
    Cursor& cursor,
    Base::Context& context,
    const Ttx::Type& root) -> const Ttx::Type* {
  cursor.consume();
  const Ttx::Type* argument = type_argument(cursor, context);
  if (argument == nullptr ||
      !cursor.require(
          Class::Type::IndexEnd, "Expected `]` after View argument."_view)) {
    return nullptr;
  }

  Managed::Bytes name = parameterized_name(context, root, *argument);
  return parameterize(context, root, *argument, Count(-1), name);
}

static auto evaluate_vec_arguments(
    Cursor& cursor,
    Base::Context& context,
    const Ttx::Type& root) -> const Ttx::Type* {
  cursor.consume();
  const Ttx::Type* argument = type_argument(cursor, context);
  if (argument == nullptr ||
      !cursor.require(
          Class::Type::PackingOp,
          "Expected `,` before Vec element count."_view)) {
    return nullptr;
  }

  const Token* extent_token =
      cursor.require(Class::Type::Numeric, "Expected Vec element count."_view);
  if (extent_token == nullptr ||
      !cursor.require(
          Class::Type::IndexEnd, "Expected `]` after Vec arguments."_view)) {
    return nullptr;
  }

  Reader::Textual extent_reader(extent_token->get_text());
  Count extent = extent_reader.read_unsigned();
  Managed::Bytes name =
      parameterized_name(context, root, *argument, extent_token->get_text());
  return parameterize(context, root, *argument, extent, name);
}

static auto evaluate_type_arguments(
    Cursor& cursor,
    Base::Context& context,
    const Ttx::Type* type) -> const Ttx::Type* {
  if (!cursor.matches(Class::Type::IndexStart)) {
    return type;
  }

  // A parameterized query must return a concrete identity. Returning the
  // generic root would make unrelated specializations compare as one type.
  if (type != nullptr && type->get_name() == "View"_view) {
    return evaluate_view_arguments(cursor, context, *type);
  }

  if (type != nullptr && type->get_name() == "Vec"_view) {
    return evaluate_vec_arguments(cursor, context, *type);
  }

  cursor.token_error("Type arguments are not supported for this type."_view);
  if (!consume_type_arguments(cursor)) {
    return nullptr;
  }

  return nullptr;
}

auto Base::Expression::Type::evaluate(Cursor& cursor, Base::Context& context)
    -> const Ttx::Type* {
  const Token* root =
      cursor.require(Class::Type::Type, "Expected Type name."_view);
  if (root == nullptr) {
    return nullptr;
  }

  return evaluate(cursor, context, root->get_text());
}

auto Base::Expression::Type::evaluate(
    Cursor& cursor,
    Base::Context& context,
    View::Bytes root_name) -> const Ttx::Type* {
  const Ttx::Type* type = context.find_type(root_name);
  if (type == nullptr) {
    type = Tetrodotoxin::Standard::Types::find_type(root_name);
  }

  return evaluate(cursor, context, type);
}

auto Base::Expression::Type::evaluate(
    Cursor& cursor,
    Base::Context& context,
    const Ttx::Type* root) -> const Ttx::Type* {
  const Ttx::Type* type = root;
  while (cursor.matches(Class::Type::TypeAccessOp)) {
    cursor.consume();

    const Token* segment = cursor.require(
        Class::Type::Type, "Expected Type name after `::`."_view);
    if (segment == nullptr) {
      return nullptr;
    }

    if (type != nullptr) {
      type = type->find_type(segment->get_text());
    }
  }

  return evaluate_type_arguments(cursor, context, type);
}
