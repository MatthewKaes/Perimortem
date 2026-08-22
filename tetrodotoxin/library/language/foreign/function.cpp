// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Foreign::Function::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ForeignFunction);
  Archive::Declaration declaration(definition);
  BAIL_IF(
      !declaration.write(writer) || !writer.write(abi) ||
      !signature.persist(writer) || !writer.finish(record));
  return True;
}

auto Language::Foreign::Function::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Foreign& host) -> Option<Function&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::ForeignFunction) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto abi = contents.read_bytes();
  auto signature = Signature::restore(contents, arena, host);
  BAIL_IF(
      !declaration || !abi || abi->is_empty() || !signature ||
      !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return arena.construct_from<Function>([&]() -> Function {
    return Function(definition, *signature, arena.proxy(*abi));
  });
}

auto Language::Foreign::Function::interpret(
    Foreign& host,
    Cursor& cursor,
    const Documentation& documentation,
    View::Bytes abi) -> Option<Function&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Token visibility = cursor.current();
  switch (visibility.get_code().get_type()) {
  case Code::Type::Private:
    cursor.create_token_error(
        visibility,
        "Private Foreign Functions are unreachable from their parent Library."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Expose:
    cursor.create_token_error(
        visibility, "Foreign Functions do not accept `expose` visibility."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Public:
    cursor.consume();
    break;
  default:
    cursor.create_token_error(
        "Foreign Functions require `public` visibility."_view);
    return {};
  }

  Token qualifier = cursor.require(
      Code::Type::Func,
      "Foreign Function declarations require the `func` keyword."_view);
  BAIL_IF(!qualifier);
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Foreign Function requires one addressable symbol name."_view);
  BAIL_IF(!name_token);

  // Signature owns the real parameter and result Layouts. Foreign changes only
  // the closing grammar from a Library Block to one terminating token.
  auto signature = Signature::interpret(cursor, host);
  BAIL_IF(!signature);
  if (signature->declares_self()) {
    cursor.create_expression_error(
        Anchor::create(Span(name_token, cursor.peek(-1))),
        "Foreign Function cannot declare a `self` receiver."_view,
        "External Callables are selected only through the source Foreign "
        "context."_view);
    return {};
  }
  if (cursor.matches(Code::Type::ScopeStart)) {
    cursor.create_token_error(
        cursor.current(),
        "Foreign Function declarations cannot contain an authored body."_view,
        "Terminate the external signature with `;`."_view);
    return {};
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Foreign Function requires one terminating `;`."_view);
  BAIL_IF(!terminator);
  auto& definition = Tetrodotoxin::Language::Definition::create_authored(
      cursor, documentation, host, {}, {}, Visibility::Public, visibility,
      name_token.caculate_text(cursor.get_source_text()), name_token, qualifier,
      Anchor::create(name_token, Span(opening, terminator)));
  Function& function = domain.construct_from<Function>(
      [&]() -> Function { return Function(definition, *signature, abi); });
  return function;
}

auto Language::Foreign::Function::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!signature.link(cursor));
  linked = True;
  return True;
}

auto Language::Foreign::Function::link_restored_declaration_signature()
    -> Bool {
  BAIL_IF(!signature.link_restored());
  linked = True;
  return True;
}

auto Language::Foreign::Function::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this) : Invalid::get_invalid();
}

auto Language::Foreign::Function::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}
