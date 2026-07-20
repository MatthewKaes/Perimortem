// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/scope.hpp"

#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Scope::define(const Ttx::Type& type) -> Bool {
  return context.define_type(type);
}

auto Library::Scope::declare_type(
    const Tetrodotoxin::Isa::Base::Declaration& definition,
    Perimortem::Utility::Range source) -> Bool {
  View::Bytes name = definition.get_name();
  if (context.find_type(name) || declarations.find(name) ||
      Tetrodotoxin::Standard::Types::is_type(name)) {
    return False;
  }

  StagedDeclaration staged(definition, source);
  declarations.insert(name, staged);
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

  StagedDeclaration& staged = entry->value;
  if (staged.get_state() == StagedDeclaration::State::Ready) {
    return staged.find_type();
  }

  if (staged.get_state() == StagedDeclaration::State::Evaluating) {
    const Ttx::Type* staged_type = staged.find_type();
    if (staged_type != nullptr) {
      return staged_type;
    }

    cursor.token_error(
        "Library type dependency cycle could not be resolved."_view);
    staged.fail();
    return nullptr;
  }

  if (staged.get_state() == StagedDeclaration::State::Failed) {
    return nullptr;
  }

  Count return_index = cursor.get_token_index();
  Bool began_evaluation = staged.begin_evaluation();
  if (!began_evaluation) {
    return nullptr;
  }

  cursor.seek_token(staged.get_source().start);

  type = materializer(cursor, *this, staged.get_declaration());
  if (type == nullptr) {
    staged.fail();
    cursor.seek_token(return_index);
    return nullptr;
  }

  Bool completed = staged.complete(*type);
  if (!completed) {
    staged.fail();
    cursor.seek_token(return_index);
    return nullptr;
  }

  Base::Definition definition(
      staged.get_declaration().get_modifier(),
      staged.get_declaration().get_attributes());
  Bool defined_type = context.define_type(*type, definition);
  if (!defined_type) {
    staged.fail();
    cursor.seek_token(return_index);
    return nullptr;
  }

  cursor.seek_token(return_index);
  return type;
}

auto Library::Scope::stage_type_reference(
    View::Bytes name,
    const Ttx::Type* type) -> Bool {
  auto* entry = declarations.find(name);
  if (!entry) {
    return False;
  }

  return entry->value.stage_type(type);
}

auto Library::Scope::seek_after_type(Cursor& cursor, View::Bytes name) const
    -> Bool {
  const auto* entry = declarations.find(name);
  if (!entry) {
    return False;
  }

  cursor.seek_token(entry->value.get_source().get_end());
  return True;
}

auto Library::Scope::find_type(View::Bytes name) const -> const Ttx::Type* {
  const Ttx::Type* type = context.find_type(name);
  if (type != nullptr) {
    return type;
  }

  type = Tetrodotoxin::Standard::Types::find_type(name);
  if (type != nullptr) {
    return type;
  }

  const auto* staged = declarations.find(name);
  return staged == nullptr ? nullptr : staged->value.find_type();
}

auto Library::Scope::resolve_type(Cursor& cursor) -> const Ttx::Type* {
  const Token* root =
      cursor.require(Code::Type::Type, "Expected Type name."_view);
  if (root == nullptr) {
    return nullptr;
  }

  return resolve_type(cursor, root->get_text());
}

auto Library::Scope::resolve_type(Cursor& cursor, View::Bytes root_name)
    -> const Ttx::Type* {
  const Ttx::Type* type = materialize_type(cursor, root_name);
  return Base::Expression::Type::evaluate(cursor, context, type);
}
