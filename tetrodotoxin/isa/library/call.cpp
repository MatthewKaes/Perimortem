// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/call.hpp"

#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Call::evaluate(Cursor& cursor, Scope& scope) -> const Call* {
  const Token& owner_token = cursor.current();
  const Ttx::Type* owner = scope.resolve_type(cursor);
  if (owner == nullptr) {
    cursor.range_error(
        owner_token, owner_token,
        "Library call target could not be resolved."_view);
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::CallOp, "Expected `->` in library call."_view)) {
    return nullptr;
  }

  const Token* function_name = cursor.require(
      Class::Type::Addressable, "Expected library function name."_view);
  if (function_name == nullptr) {
    return nullptr;
  }

  const Ttx::Type::Function* function =
      owner->find_function(function_name->get_text());
  if (function == nullptr) {
    cursor.range_error(
        *function_name, *function_name,
        "Library call function could not be resolved."_view);
    return nullptr;
  }

  const Expression::Pack* pack =
      Expression::Pack::evaluate(cursor, scope.get_context());
  if (pack == nullptr) {
    return nullptr;
  }

  return &scope.get_context().get_arena().construct<Call>(
      *owner, *function, function_name->get_text(), *pack);
}
