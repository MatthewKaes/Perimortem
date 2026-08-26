// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Package::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Allocator::Arena& transaction = cursor.get_arena();
  auto& monograph = Language::Monograph::create_authored(
      transaction, *this, documentation, source_anchor, context, library);
  auto& root = monograph.edit_library().get_source();
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, declaration_documentation, root);
    if (!definition) {
      cursor.recover_to_statement();
      continue;
    }
    auto member = Library::Interpreter::Member::parse(cursor, *definition);
    if (!member || member->get_category() !=
                       Library::Language::Types::Composite::Category::Type) {
      cursor.create_expression_error(
          definition->get_anchor(),
          "Package sources contain only Library Type definitions."_view,
          "Import source or Package roots with Alias declarations, then publish Types or namespaces."_view);
      cursor.recover_to_statement();
      continue;
    }
    root.retain_authored_definition(
        member->get_semantic(), *definition, member->get_category(), cursor);
    if (member->needs_recovery()) {
      cursor.recover_to_statement();
    }
  }
  return monograph;
}
