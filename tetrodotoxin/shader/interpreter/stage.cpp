// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/stage.hpp"

#include "tetrodotoxin/library/interpreter/declarations/signature.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Stage::is_next(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Type) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "Shader"_view;
}

auto Interpreter::Stage::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    const Documentation& documentation) -> Bool {
  BAIL_IF(!is_next(cursor));
  Token qualifier = cursor.consume();
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Shader Stage requires one Pipeline Stage name."_view);
  BAIL_IF(!name_token);
  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  auto signature =
      Library::Interpreter::Declarations::Signature::parse(cursor, program);
  BAIL_IF(!signature);
  auto& definition = Tetrodotoxin::Language::Definition::create_authored_prefix(
      cursor, documentation, program, {}, {},
      Tetrodotoxin::Language::Visibility::Public, {}, name, name_token,
      qualifier, Anchor::create(name_token, Span(qualifier, cursor.peek(-1))));
  auto& function = Library::Language::Function::create_authored(
      cursor.get_arena(), definition, *signature);
  BAIL_IF(!program.retain_authored_definition(
      function, definition,
      Library::Language::Types::Composite::Category::Callable, cursor));
  Count error_count = cursor.get_error_count();
  auto body = Library::Interpreter::Execution::Block::parse(
      cursor, function, function, function.get_host());
  BAIL_IF(!body);

  Bool accepted =
      definition.complete(qualifier, body->get_anchor().get_span().get_end()) &&
      function.complete_body(*body) && cursor.get_error_count() == error_count;
  program.retain_stage(function);
  return accepted;
}
