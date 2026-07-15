// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/virtual_machine.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"
#include "tetrodotoxin/isa/foreign/dialect.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"
#include "tetrodotoxin/isa/library/alias.hpp"
#include "tetrodotoxin/isa/library/compiler/function.hpp"
#include "tetrodotoxin/isa/library/enumeration.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "tetrodotoxin/isa/library/structure.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

using DefinitionEvaluator =
    const Ttx::Type* (*)(Cursor & cursor,
                         Library::Scope& scope,
                         const Tetrodotoxin::Isa::Base::Declaration&
                             definition);

constexpr Static::Vector<Class::Type, 4> library_modifiers = {{
  Class::Type::Public,
  Class::Type::Private,
  Class::Type::Expose,
  Class::Type::Const,
}};

constexpr Static::Vector<Pair<View::Bytes, DefinitionEvaluator>, 5>
    library_sub_isas = {{
      {
        Library::Alias::get_name(),
        Library::Alias::evaluate,
      },
      {
        Library::Enumeration::get_name(),
        Library::Enumeration::evaluate,
      },
      {
        Library::Structure::get_name(),
        Library::Structure::evaluate,
      },
      {
        // Object shares aggregate syntax. Lifetime policy belongs to the
        // runtime implementation, not this parser.
        "object"_view,
        Library::Structure::evaluate,
      },
      {
        Foreign::Dialect::get_name(),
        Foreign::Dialect::evaluate,
      },
    }};

static auto materialize_library_type(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition)
    -> const Ttx::Type* {
  const auto* handler =
      Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
          definition.get_kind());
  return handler == nullptr ? nullptr : (*handler)(cursor, scope, definition);
}

static auto predeclare_library_definitions(
    Cursor& cursor,
    Library::Scope& scope) -> Bool {
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
    Bool attributes_evaluated =
        Base::Attribute::evaluate_all(cursor, attributes);
    if (!attributes_evaluated) {
      return False;
    }

    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    if (!cursor.is_one_of(library_modifiers)) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return False;
      }

      continue;
    }

    Class::Type modifier = cursor.current().get_class().get_type();
    cursor.consume();
    if (cursor.matches(Class::Type::Func)) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return False;
      }

      continue;
    }

    const Token& name = cursor.current();
    if (name.get_class() != Class::Type::Type) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return False;
      }

      continue;
    }

    cursor.consume();
    if (!cursor.matches(Class::Type::Define)) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return False;
      }

      continue;
    }

    cursor.consume();
    const Token& kind = cursor.current();
    if (Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
            kind.get_text()) == nullptr) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return False;
      }

      continue;
    }

    Tetrodotoxin::Isa::Base::Declaration definition(
        documentation, modifier, name.get_class().get_type(), name.get_text(),
        kind.get_class().get_type(), kind.get_text(), attributes.get_view());
    cursor.consume();
    Range source = {cursor.get_token_index(), 0};
    Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
    if (!recovered) {
      return False;
    }

    source.size = cursor.get_token_index() - source.start;
    Bool declared = scope.declare_type(definition, source);
    if (!declared) {
      cursor.range_error(
          name, name, "Library type name is already defined."_view);
      return False;
    }
  }

  return True;
}

auto Library::VirtualMachine::evaluate_definition(
    Cursor& cursor,
    Library::Scope& scope,
    Ttx::Documentation documentation,
    View::Vector<Ttx::Attribute> attributes,
    Managed::Vector<Ttx::Member>& members,
    Managed::Vector<Base::Definition>& member_definitions,
    Managed::Vector<Ttx::Type::Reference>& types,
    Managed::Vector<Ttx::Function>& functions,
    Managed::Vector<Range>& function_sources,
    Managed::Vector<Base::Definition>& function_definitions) -> Bool {
  Class::Type modifier = Base::Modifier::evaluate(
      cursor, library_modifiers,
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose, const}"_view);
  if (modifier == Class::Type::Unknown) {
    return False;
  }

  if (cursor.matches(Class::Type::Func)) {
    Range source;
    Ttx::Function function =
        Library::Function::evaluate(cursor, scope, documentation, source);
    if (function.is_empty()) {
      return False;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("Library function name is already defined."_view);
        return False;
      }
    }

    functions.insert(function);
    function_sources.insert(source);
    function_definitions.insert(Base::Definition(modifier, attributes));
    return True;
  }

  Tetrodotoxin::Isa::Base::Declaration definition =
      Tetrodotoxin::Isa::Base::Declaration::evaluate_after_modifier(
          cursor, documentation, modifier,
          {{Class::Type::Type, Class::Type::Addressable}},
          {{Class::Type::Addressable, Class::Type::Type, Class::Type::Alias,
            Class::Type::Func}},
          attributes);
  if (definition.is_empty()) {
    return False;
  }

  if (definition.has_addressable_name()) {
    Base::Definition member_implementation;
    const Ttx::Member* member = Library::Addressable::evaluate(
        cursor, scope, definition, member_implementation);
    if (member == nullptr) {
      return False;
    }

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].get_name() == member->get_name()) {
        cursor.token_error("Library member name is already defined."_view);
        return False;
      }
    }

    members.insert(*member);
    member_definitions.insert(member_implementation);
    return True;
  }

  const auto* handler =
      Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
          definition.get_kind());
  if (handler == nullptr) {
    Managed::Bytes message(cursor.get_arena());
    message.concat(
        "Definition name provided is not one of the known types {"_view);
    for (Count i = 0; i < library_sub_isas.get_size(); i++) {
      if (i != 0) {
        message.concat(", "_view);
      }

      message.concat(library_sub_isas[i].key);
    }

    message.concat("}"_view);
    cursor.token_error(message.get_view());
    return False;
  }

  const Ttx::Type* type = scope.materialize_type(cursor, definition.get_name());
  if (type == nullptr) {
    return False;
  }

  Bool positioned = scope.seek_after_type(cursor, definition.get_name());
  if (!positioned) {
    Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
    if (!recovered) {
      return False;
    }
  }

  types.insert(Ttx::Type::Reference(*type));
  return True;
}

auto Library::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Member> members(context.get_arena());
  Managed::Vector<Base::Definition> member_definitions(context.get_arena());
  Managed::Vector<Ttx::Type::Reference> types(context.get_arena());
  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Managed::Vector<Range> function_sources(context.get_arena());
  Managed::Vector<Base::Definition> function_definitions(context.get_arena());
  Library::Scope scope(context, materialize_library_type);
  Count body_start = cursor.get_token_index();
  Bool predeclared = predeclare_library_definitions(cursor, scope);
  if (!predeclared) {
    return nullptr;
  }

  cursor.seek_token(body_start);
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> source_attributes(context.get_arena());
    Bool attributes_evaluated =
        Base::Attribute::evaluate_all(cursor, source_attributes);
    if (!attributes_evaluated) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    Bool evaluated = evaluate_definition(
        cursor, scope, documentation, source_attributes.get_view(), members,
        member_definitions, types, functions, function_sources,
        function_definitions);
    if (!evaluated) {
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor, True);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }
  }

  if (!cursor.get_errors().is_empty()) {
    return nullptr;
  }

  auto& type = context.get_arena().construct<Ttx::Type>(
      Library::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      functions.get_view());
  for (Count i = 0; i < members.get_size(); i++) {
    Bool implementation_defined =
        context.define_implementation(members[i], member_definitions[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  Bool published = Library::Compiler::Function::publish(
      cursor, scope, type, Library::VirtualMachine::get_name(),
      type.get_type_functions(), function_sources.get_view(),
      function_definitions.get_view(), False);
  if (!published) {
    return nullptr;
  }

  return &type;
}
