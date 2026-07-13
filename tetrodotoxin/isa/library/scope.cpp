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

  StagedDeclaration staged;
  staged.declaration = definition;
  staged.source = source;
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
  if (staged.state == StagedDeclaration::State::Ready) {
    return staged.type;
  }

  if (staged.state == StagedDeclaration::State::Evaluating) {
    if (staged.type != nullptr) {
      return staged.type;
    }

    cursor.token_error(
        "Library type dependency cycle could not be resolved."_view);
    staged.state = StagedDeclaration::State::Failed;
    return nullptr;
  }

  if (staged.state == StagedDeclaration::State::Failed) {
    return nullptr;
  }

  Count return_index = cursor.get_token_index();
  staged.state = StagedDeclaration::State::Evaluating;
  cursor.seek_token(staged.source.start);

  type = materializer(cursor, *this, staged.declaration);
  if (type == nullptr || !context.define_type(*type)) {
    staged.state = StagedDeclaration::State::Failed;
    cursor.seek_token(return_index);
    return nullptr;
  }

  staged.type = type;
  staged.state = StagedDeclaration::State::Ready;
  cursor.seek_token(return_index);
  return type;
}

auto Library::Scope::stage_type_reference(
    View::Bytes name,
    const Ttx::Type& type) -> Bool {
  auto* entry = declarations.find(name);
  if (!entry || entry->value.state != StagedDeclaration::State::Evaluating) {
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

  cursor.seek_token(entry->value.source.get_end());
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
  return staged == nullptr ? nullptr : staged->value.type;
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
  return Base::Expression::Type::evaluate(cursor, context, type);
}
