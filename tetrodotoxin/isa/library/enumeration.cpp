// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/enumeration.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"
#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto parse_storage_type(Library::Scope& scope, Cursor& cursor)
    -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::IndexStart,
          "Expected `[` before library enum storage type."_view)) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* storage = scope.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::IndexEnd,
          "Expected `]` after library enum storage type."_view)) {
    return nullptr;
  }

  if (storage == nullptr) {
    cursor.token_error("Library enum storage type could not be resolved."_view);
  }
  return storage;
}

static auto consume_pack_value(Cursor& cursor) -> Bool {
  Count scope_depth = 0;
  Count packing_depth = 0;
  Count index_depth = 0;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeStart)) {
      scope_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::ScopeEnd)) {
      if (scope_depth > 0) {
        scope_depth--;
      }
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingStart)) {
      packing_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      if (scope_depth == 0 && packing_depth == 0 && index_depth == 0) {
        return True;
      }
      if (packing_depth > 0) {
        packing_depth--;
      }
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexStart) ||
        cursor.matches(Class::Type::SliceOp) ||
        cursor.matches(Class::Type::SwizzleOp)) {
      index_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexEnd)) {
      if (index_depth > 0) {
        index_depth--;
      }
      cursor.consume();
      continue;
    }

    if (scope_depth == 0 && packing_depth == 0 && index_depth == 0 &&
        cursor.matches(Class::Type::PackingOp)) {
      return True;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `)` after library enum cases."_view);
  return False;
}

static auto has_member(
    View::Vector<Ttx::Type::Member> members,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i].get_name() == name) {
      return True;
    }
  }
  return False;
}

static auto has_function(
    View::Vector<Ttx::Type::Function> functions,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i].get_name() == name) {
      return True;
    }
  }
  return False;
}

static auto insert_enum_case(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Member>& members,
    View::Bytes name,
    const Ttx::Type& storage,
    Ttx::Documentation documentation) -> Bool {
  if (has_member(members.get_view(), name)) {
    cursor.token_error("Library enum case name is already defined."_view);
    return False;
  }

  members.insert(Ttx::Type::Member(name, storage, documentation));
  return True;
}

static auto evaluate_packed_cases(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition,
    const Ttx::Type& storage) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::PackingStart,
          "Expected `(` before library enum cases."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(scope.get_context().get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::PackingEnd)) {
    if (!cursor.require(
            Class::Type::AddressOp,
            "Expected `.` before library enum case name."_view)) {
      valid = False;
      if (!consume_pack_value(cursor)) {
        return nullptr;
      }
    } else {
      const Token* name = cursor.require(
          Class::Type::Addressable, "Expected library enum case name."_view);
      if (name == nullptr) {
        valid = False;
        if (!consume_pack_value(cursor)) {
          return nullptr;
        }
      } else if (!cursor.require(
                     Class::Type::Assign,
                     "Expected `=` after library enum case name."_view)) {
        valid = False;
        if (!consume_pack_value(cursor)) {
          return nullptr;
        }
      } else if (!consume_pack_value(cursor)) {
        return nullptr;
      } else if (!insert_enum_case(
                     cursor, members, name->get_text(), storage,
                     definition.get_documentation())) {
        valid = False;
      }
    }

    if (cursor.matches(Class::Type::PackingOp)) {
      cursor.consume();
      continue;
    }

    if (!cursor.matches(Class::Type::PackingEnd)) {
      cursor.token_error("Expected `,` or `)` after library enum case."_view);
      valid = False;
      if (!consume_pack_value(cursor)) {
        return nullptr;
      }
    }
  }

  if (!cursor.require(
          Class::Type::PackingEnd,
          "Expected `)` after library enum cases."_view)) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after library enum declaration."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  attributes.insert({"isa"_view, "Enum"_view});
  auto& type = scope.get_context().get_arena().construct<Ttx::Type>(
      definition.get_name(), members.get_view(),
      View::Vector<const Ttx::Type*>(), View::Vector<Ttx::Type::Function>(),
      definition.get_documentation(), attributes.get_view());
  return &type;
}

static auto evaluate_scoped_cases(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition,
    const Ttx::Type& storage) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library enum storage type."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(scope.get_context().get_arena());
  Managed::Vector<Ttx::Type::Function> functions(
      scope.get_context().get_arena());
  Ttx::Type& type = scope.get_context().get_arena().allocate<Ttx::Type>();
  if (!scope.stage_type_reference(definition.get_name(), type)) {
    cursor.token_error("Library enum type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Boot::Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::Addressable)) {
      View::Bytes name = cursor.current().get_text();
      cursor.consume();
      if (!cursor.matches(Class::Type::Assign)) {
        cursor.token_error("Expected `=` after library enum case name."_view);
        valid = False;
        if (!Library::Syntax::consume_declaration_tail(cursor)) {
          return nullptr;
        }
        continue;
      }

      if (!Library::Syntax::consume_initializer(
              cursor,
              "Expected `;` after library enum case initializer."_view)) {
        return nullptr;
      }

      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after library enum case."_view)) {
        return nullptr;
      }

      if (!insert_enum_case(cursor, members, name, storage, documentation)) {
        valid = False;
      }
      continue;
    }

    Modifier modifier = Modifier::evaluate(
        cursor,
        {{Class::Type::Public, Class::Type::Private, Class::Type::Expose}},
        "Expected enum body to contain a case name or function modifier."_view);
    if (!modifier.is_valid()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    if (!cursor.matches(Class::Type::Func)) {
      cursor.token_error("Expected enum member function."_view);
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    Ttx::Type::Function function =
        Library::Function::evaluate(scope, cursor, documentation);
    if (function.is_empty()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    if (has_function(functions.get_view(), function.get_name())) {
      cursor.token_error("Library function name is already defined."_view);
      valid = False;
      continue;
    }

    functions.insert(function);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library enum declaration."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  attributes.insert({"isa"_view, "Enum"_view});
  new (&type) Ttx::Type(
      definition.get_name(), members.get_view(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      definition.get_documentation(), attributes.get_view());
  return &type;
}

auto Library::Enumeration::evaluate(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  const Ttx::Type* storage = parse_storage_type(scope, cursor);
  if (storage == nullptr) {
    return nullptr;
  }

  if (cursor.matches(Class::Type::PackingStart)) {
    return evaluate_packed_cases(scope, cursor, definition, *storage);
  }

  if (cursor.matches(Class::Type::ScopeStart)) {
    return evaluate_scoped_cases(scope, cursor, definition, *storage);
  }

  cursor.token_error(
      "Expected `(` or `{` after library enum storage type."_view);
  return nullptr;
}
