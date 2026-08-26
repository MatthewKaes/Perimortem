// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/stage.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/layout.hpp"
#include "tetrodotoxin/library/language/signature.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Stage::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_name_token().get_code() != Code::Type::Addressable ||
      definition.get_visibility() !=
          Tetrodotoxin::Language::Visibility::Public ||
      !definition.get_modifiers().is_empty() ||
      !definition.get_attributes().is_empty()) {
    cursor.create_expression_error(
        definition.get_anchor(),
        "Shader Stage bodies require one public Function name."_view,
        "Keep the Render signature and interface facts on the selected contract."_view);
    return False;
  }

  Token qualifier = cursor.require(
      Code::Type::Func, "Shader Stage bodies use the `func` qualifier."_view);
  BAIL_IF(!qualifier);
  if (cursor.matches(Code::Type::Assign)) {
    cursor.create_token_error(
        "Shader Stage bodies inherit their complete Render signature."_view,
        "Remove the repeated parameter and result Layouts."_view);
    return False;
  }

  Managed::Vector<Library::Language::Model::Layout::Slot> parameter_slots(
      cursor.get_arena());
  Managed::Vector<Library::Language::Model::Layout::Slot> result_slots(
      cursor.get_arena());
  auto& parameters = Library::Language::Model::Layout::create_authored(
      cursor.get_arena(), parameter_slots, Anchor::create(Span()), True);
  auto& results = Library::Language::Model::Layout::create_authored(
      cursor.get_arena(), result_slots, Anchor::create(Span()), False);
  auto& signature = Library::Language::Signature::create_authored(
      cursor.get_arena(), program, parameters, results);
  auto& function = Library::Language::Function::create_authored(
      cursor.get_arena(), definition, signature);
  Count error_count = cursor.get_error_count();
  auto body = Library::Interpreter::Execution::Block::parse(
      cursor, function, function, function.get_host());
  BAIL_IF(!body);

  Bool accepted =
      definition.complete(qualifier, body->get_anchor().get_span().get_end()) &&
      function.complete_body(*body) && cursor.get_error_count() == error_count;
  BAIL_IF(!program.retain_authored_definition(
      function, definition,
      Library::Language::Types::Composite::Category::Callable, cursor));
  program.retain_stage(function, parameters, results);
  return accepted;
}
