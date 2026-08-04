// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/expression.hpp"

#include "tetrodotoxin/library/language/operations/slice.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

static auto parse_expression(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context)
    -> Option<const Library::Language::Expression&> {
  auto primary = Library::Language::Parser::Literal::parse(
      domain, materializations, cursor, source_context);
  if (!primary) {
    return {};
  }

  Reference<Library::Language::Expression> expression(*primary);
  while (True) {
    // Expression owns only precedence and operator selection. Each selected
    // operator consumes its complete grammar and returns one semantic edge.
    switch (cursor.get_code().get_type()) {
    case Code::Type::SliceOp: {
      auto slice = Library::Language::Operations::Slice::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!slice) {
        return {};
      }

      expression = *slice;
      break;
    }
    default:
      return expression.get();
    }
  }
}

auto Library::Language::Parser::Expression::parse(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<const Language::Expression&> {
  Cursor transaction = cursor;
  auto parsed =
      parse_expression(domain, materializations, transaction, source_context);
  if (!parsed) {
    return {};
  }

  // Only this final synchronization changes caller position. Nested operands
  // and partial postfix chains remain private to the transaction Cursor.
  cursor.sync(transaction);
  return *parsed;
}
