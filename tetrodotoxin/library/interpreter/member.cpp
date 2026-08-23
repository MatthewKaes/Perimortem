// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/member.hpp"

#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/interpreter/declarations/alias.hpp"
#include "tetrodotoxin/library/interpreter/declarations/field.hpp"
#include "tetrodotoxin/library/interpreter/declarations/function.hpp"
#include "tetrodotoxin/library/interpreter/types/structure.hpp"
#include "tetrodotoxin/library/interpreter/types/object.hpp"
#include "tetrodotoxin/library/interpreter/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;

auto Interpreter::Member::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Result> {
  Token qualifier = definition.get_qualifier();
  switch (qualifier.get_code().get_type()) {
  case Code::Type::Type:
  case Code::Type::Assign: {
    auto field = Declarations::Field::parse(cursor, definition);
    BAIL_IF(!field);
    return Result(*field, Language::Types::Composite::Category::Addressable);
  }
  case Code::Type::Alias: {
    auto alias = Declarations::Alias::parse(cursor, definition);
    BAIL_IF(!alias);
    return Result(*alias, Language::Types::Composite::Category::Type);
  }
  case Code::Type::Func: {
    auto function = Declarations::Function::parse(cursor, definition);
    BAIL_IF(!function);
    return Result(*function, Language::Types::Composite::Category::Callable);
  }
  case Code::Type::Enum: {
    auto enumeration = Interpreter::Types::Enumeration::parse(cursor, definition);
    BAIL_IF(!enumeration);
    return Result(*enumeration, Language::Types::Composite::Category::Type);
  }
  case Code::Type::Struct: {
    auto structure = Interpreter::Types::Structure::parse(cursor, definition);
    BAIL_IF(!structure);
    return Result(*structure, Language::Types::Composite::Category::Type);
  }
  case Code::Type::Object: {
    auto object = Interpreter::Types::Object::parse(cursor, definition);
    BAIL_IF(!object);
    return Result(*object, Language::Types::Composite::Category::Type);
  }
  default:
    cursor.create_token_error(
        qualifier,
        "Library members require a Type, `alias`, `enum`, `struct`, `object`, "
        "`func`, or inferred initializer qualifier."_view);
    return {};
  }
}
