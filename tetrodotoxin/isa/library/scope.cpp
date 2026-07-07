// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/scope.hpp"

#include "tetrodotoxin/isa/expression/type.hpp"
#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Scope::define(const Ttx::Type& type) -> Bool {
  return context.define_type(type);
}

auto Library::Scope::declare_type(
    const Tetrodotoxin::Isa::Definition& definition,
    Count body_index,
    Count next_index) -> Bool {
  View::Bytes name = definition.get_name();
  if (context.find_type(name) || declarations.find(name) ||
      Tetrodotoxin::Standard::Types::is_type(name)) {
    return False;
  }

  Declaration declaration;
  declaration.definition = definition;
  declaration.body_index = body_index;
  declaration.next_index = next_index;
  declarations.insert(name, declaration);
  return True;
}

auto Library::Scope::materialize_type(Cursor& cursor, View::Bytes name)
    -> const Ttx::Type* {
  const Ttx::Type* type = context.find_type(name);
  if (type != nullptr) {
    return type;
  }

  type = Tetrodotoxin::Standard::Types::find_type(name);
  if (type != nullptr) {
    return type;
  }

  auto* entry = declarations.find(name);
  if (!entry || materializer == nullptr) {
    return nullptr;
  }

  Declaration& declaration = entry->value;
  if (declaration.state == DeclarationState::Ready) {
    return declaration.type;
  }

  if (declaration.state == DeclarationState::Evaluating) {
    if (declaration.type != nullptr) {
      return declaration.type;
    }

    cursor.token_error(
        "Library type dependency cycle could not be resolved."_view);
    declaration.state = DeclarationState::Failed;
    return nullptr;
  }

  if (declaration.state == DeclarationState::Failed) {
    return nullptr;
  }

  Count return_index = cursor.get_token_index();
  declaration.state = DeclarationState::Evaluating;
  cursor.seek_token(declaration.body_index);

  type = materializer(cursor, *this, declaration.definition);
  if (type == nullptr || !context.define_type(*type)) {
    declaration.state = DeclarationState::Failed;
    cursor.seek_token(return_index);
    return nullptr;
  }

  declaration.type = type;
  declaration.state = DeclarationState::Ready;
  cursor.seek_token(return_index);
  return type;
}

auto Library::Scope::stage_type_reference(
    View::Bytes name,
    const Ttx::Type& type) -> Bool {
  auto* entry = declarations.find(name);
  if (!entry || entry->value.state != DeclarationState::Evaluating) {
    return False;
  }

  entry->value.type = &type;
  return True;
}

auto Library::Scope::seek_after_type(Cursor& cursor, View::Bytes name) const
    -> Bool {
  const auto* entry = declarations.find(name);
  if (!entry) {
    return False;
  }

  cursor.seek_token(entry->value.next_index);
  return True;
}

auto Library::Scope::find_type(View::Bytes name) const -> const Ttx::Type* {
  return context.find_type(name);
}

auto Library::Scope::resolve_type(Cursor& cursor) -> const Ttx::Type* {
  const Token* root =
      cursor.require(Class::Type::Type, "Expected Type name."_view);
  if (root == nullptr) {
    return nullptr;
  }

  return resolve_type(cursor, root->get_text());
}

auto Library::Scope::resolve_type(Cursor& cursor, View::Bytes root_name)
    -> const Ttx::Type* {
  const Ttx::Type* type = materialize_type(cursor, root_name);
  return Expression::Type::evaluate(cursor, context, type);
}
