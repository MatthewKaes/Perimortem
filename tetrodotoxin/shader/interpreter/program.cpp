// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/program.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/type_reference.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/shader/interpreter/bridge.hpp"
#include "tetrodotoxin/shader/interpreter/stage.hpp"
#include "tetrodotoxin/shader/interpreter/uniform.hpp"
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
  if (!definition.get_modifiers().is_empty() ||
      !definition.get_attributes().is_empty()) {
    cursor.create_expression_error(
        definition.get_anchor(),
        "Shader Programs do not accept modifiers or pipeline Attributes."_view,
        "Keep fixed pipeline meaning on the selected Render contract."_view);
    return False;
  }
  return True;
}

static auto begins_uniform(const Cursor& cursor) -> Bool {
  S64 offset = 0;
  Code::Type visibility = cursor.peek(offset).get_code().get_type();
  BAIL_IF(
      visibility != Code::Type::Public && visibility != Code::Type::Private &&
      visibility != Code::Type::Expose);
  offset++;
  while (cursor.peek(offset).get_code().is_evaluation_modifier()) {
    offset++;
  }
  BAIL_IF(
      cursor.peek(offset).get_code() != Code::Type::Addressable &&
      cursor.peek(offset).get_code() != Code::Type::Type);
  offset++;
  BAIL_IF(cursor.peek(offset).get_code() != Code::Type::Define);
  offset++;
  return cursor.peek(offset).caculate_text(cursor.get_source_text()) ==
         "uniform"_view;
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

  auto& program = Shader::Language::Program::create_authored(
      cursor.get_arena(), definition, *contract, monograph);
  BAIL_IF(!program.initialize_runtime_surface());

  // Program enters the real Library child before its body is interpreted. Its
  // generated runtime Types and authored Stage bodies then share the ordinary
  // Library completion barriers.
  BAIL_IF(!monograph.edit_library().get_source().retain_definition(
      program, Library::Language::Types::Composite::Category::Type, True));
  BAIL_IF(!monograph.retain_program(program));
  cursor.get_associations().create(definition.get_name_anchor(), program);

  while (!cursor.matches(Code::Type::ScopeEnd) &&
         !cursor.matches(Code::Type::Terminal)) {
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    Bool uniform = begins_uniform(cursor);
    Abstract& host = uniform ? static_cast<Abstract&>(program.edit_parameters())
                             : static_cast<Abstract&>(program);
    auto member_definition =
        Tetrodotoxin::Language::Definition::parse(cursor, documentation, host);
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
      } else if (qualifier_name == "uniform"_view) {
        parsed =
            Interpreter::Uniform::parse(program, cursor, *member_definition);
      } else if (Interpreter::Value::matches(*member_definition, cursor)) {
        parsed = Interpreter::Value::parse(program, cursor, *member_definition);
      } else {
        auto report = cursor.create_report(member_definition->get_anchor());
        report
            << "Shader Programs author only Stage bodies, storage values, uniforms, and Bridges, not `"_view
            << qualifier_name << "`."_view;
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
  program.complete_authored_body();
  return completed;
}
