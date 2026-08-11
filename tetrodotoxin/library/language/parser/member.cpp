// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/member.hpp"

#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Parser::Member::parse(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Abstract&> {
  Token qualifier = definition.get_qualifier();
  switch (qualifier.get_code().get_type()) {
  case Code::Type::Type:
  case Code::Type::Assign: {
    auto field = Field::interpret(domain, materializations, cursor, definition);
    BAIL_IF(!field);
    return *field;
  }
  case Code::Type::Alias: {
    auto alias = Alias::interpret(domain, cursor, definition);
    BAIL_IF(!alias);
    return *alias;
  }
  case Code::Type::Func: {
    auto function = Function::reserve(domain, cursor, definition);
    BAIL_IF(!function || !function->complete(cursor, materializations));
    return *function;
  }
  case Code::Type::Enum: {
    auto enumeration =
        Types::Enumeration::interpret(domain, cursor, definition);
    BAIL_IF(!enumeration);
    return *enumeration;
  }
  case Code::Type::Struct: {
    auto structure = Types::Structure::interpret(domain, cursor, definition);
    BAIL_IF(!structure);
    return *structure;
  }
  case Code::Type::Object: {
    auto object = Types::Object::interpret(domain, cursor, definition);
    BAIL_IF(!object);
    return *object;
  }
  default:
    cursor.create_token_error(
        qualifier,
        "Library members require a Type, `alias`, `enum`, `struct`, `object`, "
        "`func`, or inferred initializer qualifier."_view);
    return {};
  }
}
