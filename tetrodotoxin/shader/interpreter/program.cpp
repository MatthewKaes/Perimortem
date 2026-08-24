// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/program.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/type_reference.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/shader/interpreter/bridge.hpp"
#include "tetrodotoxin/shader/interpreter/stage.hpp"
#include "tetrodotoxin/shader/interpreter/value.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto has_program_shape(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  Token qualifier = definition.get_qualifier();
  if (qualifier.caculate_text(cursor.get_source_text()) != "shader"_view) {
    cursor.create_token_error(
        qualifier,
        "Shader Program definitions require the `shader` qualifier."_view);
    return False;
  }
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(), "Shader Programs use one Type name."_view);
    return False;
  }
  if (definition.get_visibility() !=
      Tetrodotoxin::Language::Visibility::Public) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Shader Programs use public visibility."_view);
    return False;
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Shader Programs do not accept evaluation modifiers."_view);
    return False;
  }
  return Render::Language::Attributes::validate(
      cursor, definition.get_attributes(),
      Render::Language::Attributes::Placement::Structure);
}

static auto parse_library_type(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto member = Library::Interpreter::Member::parse(cursor, definition);
  BAIL_IF(!member);

  Abstract& semantic = member->get_semantic();
  Bool admitted = semantic.is<Library::Language::Alias>() ||
                  semantic.is<Library::Language::Types::Structure>();
  if (!admitted) {
    cursor.create_expression_error(
        definition.get_anchor(),
        "Shader Programs admit Library Alias and Structure Types here."_view,
        "Use `func`, value, resource, push, or bridge for other declarations."_view);
  }
  Bool retained =
      admitted && program.retain_authored_definition(
                      semantic, definition, member->get_category(), cursor);
  if (member->needs_recovery()) {
    cursor.recover_to_scoped_statement();
  }
  return retained && member->is_accepted();
}

auto Interpreter::Program::parse(
    Shader::Language::Monograph& monograph,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(!has_program_shape(cursor, definition));
  Token qualifier = cursor.consume();
  auto contract = Tetrodotoxin::Language::Parser::TypeReference::parse(cursor);
  BAIL_IF(!contract);
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart, "Shader Program requires one `{}` body."_view));

  auto& program = Shader::Language::Program::create(
      cursor.get_arena(), definition, *contract, monograph);

  // The Program is admitted directly into the child Source as a Library Type.
  // Its executable declarations therefore participate in the ordinary Library
  // completion barriers instead of a Shader specific body graph.
  BAIL_IF(!monograph.edit_library().get_source().bind_static(
      program, Library::Language::Types::Composite::Category::Type));
  BAIL_IF(!monograph.retain_program(program));
  cursor.get_associations().create(definition.get_name_anchor(), program);

  while (!cursor.matches(Code::Type::ScopeEnd) &&
         !cursor.matches(Code::Type::Terminal)) {
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto member_definition = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, program);
    Bool parsed = False;
    if (member_definition) {
      Token member_qualifier = member_definition->get_qualifier();
      View::Bytes qualifier_name =
          member_qualifier.caculate_text(cursor.get_source_text());
      if (member_qualifier.get_code() == Code::Type::Func) {
        parsed = Interpreter::Stage::parse(program, cursor, *member_definition);
      } else if (qualifier_name == "bridge"_view) {
        parsed =
            Interpreter::Bridge::parse(monograph, cursor, *member_definition);
      } else if (Interpreter::Value::matches(*member_definition, cursor)) {
        parsed = Interpreter::Value::parse(program, cursor, *member_definition);
      } else {
        parsed = parse_library_type(program, cursor, *member_definition);
      }
    }
    if (!parsed) {
      cursor.recover_to_scoped_statement();
    }
  }
  Token closing = cursor.require(
      Code::Type::ScopeEnd, "Shader Program requires one closing `}`."_view);
  BAIL_IF(!closing);
  Bool completed = definition.complete(qualifier, closing);

  // Structure owns the body boundary used by Library completion. Closing it
  // here gives later Type and Function barriers one ordinary Composite rather
  // than a special Shader publication path.
  program.complete_body();
  return completed;
}
