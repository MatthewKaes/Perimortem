// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/value.hpp"

#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/shader/language/binding.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Value::matches(
    const Tetrodotoxin::Language::Definition& definition,
    const Cursor& cursor) -> Bool {
  Code::Type qualifier = definition.get_qualifier().get_code().get_type();
  if (qualifier == Code::Type::Type || qualifier == Code::Type::Assign) {
    return True;
  }
  View::Bytes name =
      definition.get_qualifier().caculate_text(cursor.get_source_text());
  return name == "push"_view || name == "resource"_view;
}

static auto qualifier_name(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> View::Bytes {
  return definition.get_qualifier().caculate_text(cursor.get_source_text());
}

static auto retain_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Library::Language::Field& field,
    Render::Language::Binding::Kind kind,
    Bool accepted) -> Bool {
  Bool retained = program.retain_authored_definition(
      field, definition,
      Library::Language::Types::Composite::Category::Addressable, cursor);
  if (retained) {
    program.retain_shader_binding(field, kind);
  }
  return retained && accepted;
}

static auto parse_library_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto member = Library::Interpreter::Member::parse(cursor, definition);
  BAIL_IF(!member);
  auto field = member->get_semantic().select<Library::Language::Field>();
  if (!field) {
    cursor.create_expression_error(
        definition.get_anchor(),
        "Shader value definitions create one Library Field."_view);
    return False;
  }

  Bool attributes_valid = Render::Language::Attributes::validate(
      cursor, definition.get_attributes(),
      Render::Language::Attributes::Placement::Value);
  Render::Language::Binding::Kind kind =
      field->get_writability() == Library::Language::Writability::Constant
          ? Render::Language::Binding::Kind::Constant
          : Render::Language::Binding::Kind::Value;
  Bool retained = retain_value(
      program, cursor, definition, *field, kind, member->is_accepted());
  if (member->needs_recovery()) {
    cursor.recover_to_scoped_statement();
  }
  return attributes_valid && retained;
}

static auto parse_shader_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    View::Bytes qualifier) -> Bool {
  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Shader storage definitions use one addressable name."_view);
    return False;
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Shader resource and push definitions do not accept evaluation modifiers."_view);
    return False;
  }
  Token qualifier_token = cursor.consume();
  auto type = Library::Interpreter::TypeReference::parse(program, cursor);
  BAIL_IF(!type);
  Option<Library::Language::Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    initializer = Library::Interpreter::Pack::parse(program, cursor);
    BAIL_IF(!initializer);
  }
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Shader storage definitions require one trailing `;`."_view);
  BAIL_IF(!closing);

  Render::Language::Binding::Kind kind =
      qualifier == "push"_view ? Render::Language::Binding::Kind::Push
                               : Render::Language::Binding::Kind::Resource;
  Render::Language::Attributes::Placement placement =
      qualifier == "push"_view
          ? Render::Language::Attributes::Placement::Push
          : Render::Language::Attributes::Placement::Resource;
  Bool attributes_valid = Render::Language::Attributes::validate(
      cursor, definition.get_attributes(), placement);
  auto& field = Library::Language::Field::create_authored(
      cursor.get_arena(), definition, Library::Language::Writability::Full,
      *type, initializer);
  Bool completed = definition.complete(qualifier_token, closing);
  Bool retained =
      retain_value(program, cursor, definition, field, kind, completed);
  return attributes_valid && retained;
}

auto Interpreter::Value::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  View::Bytes qualifier = qualifier_name(definition, cursor);
  return qualifier == "push"_view || qualifier == "resource"_view
             ? parse_shader_value(program, cursor, definition, qualifier)
             : parse_library_value(program, cursor, definition);
}
