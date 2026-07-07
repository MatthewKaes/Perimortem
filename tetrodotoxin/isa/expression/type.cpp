// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression/type.hpp"

#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
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

static auto evaluate_type_arguments(Cursor& cursor, const Ttx::Type* type)
    -> const Ttx::Type* {
  if (!cursor.matches(Class::Type::IndexStart)) {
    return type;
  }

  // The current standard table only publishes the concrete `View[Bytes]`
  // specialization. Other type arguments are consumed so richer generic syntax
  // stays parseable until generic type materialization exists.
  if (type != nullptr && type->get_name() == "View"_view) {
    const Count start = cursor.get_token_index();
    cursor.consume();
    if (cursor.matches(Class::Type::Type) &&
        cursor.current().get_text() == "Bytes"_view) {
      cursor.consume();
      if (cursor.matches(Class::Type::IndexEnd)) {
        cursor.consume();
        return Tetrodotoxin::Standard::Types::find_type("View[Bytes]"_view);
      }
    }
    cursor.seek_token(start);
  }

  return consume_type_arguments(cursor) ? type : nullptr;
}

auto Expression::Type::evaluate(Cursor& cursor, Context& context)
    -> const Ttx::Type* {
  const Token* root =
      cursor.require(Class::Type::Type, "Expected Type name."_view);
  if (root == nullptr) {
    return nullptr;
  }

  return evaluate(cursor, context, root->get_text());
}

auto Expression::Type::evaluate(
    Cursor& cursor,
    Context& context,
    View::Bytes root_name) -> const Ttx::Type* {
  const Ttx::Type* type = context.find_type(root_name);
  if (type == nullptr) {
    type = Tetrodotoxin::Standard::Types::find_type(root_name);
  }

  return evaluate(cursor, context, type);
}

auto Expression::Type::evaluate(
    Cursor& cursor,
    Context&,
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

  return evaluate_type_arguments(cursor, type);
}
