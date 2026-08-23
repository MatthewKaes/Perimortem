// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/function.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Function::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Function);
  Archive::Declaration declaration(definition);
  return declaration.write(writer) && signature.persist(writer) &&
         writer.finish(record);
}

auto Language::Function::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Function&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Function) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto signature = Signature::restore(contents, arena, host);
  BAIL_IF(!declaration || !signature || !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return arena.construct_from<Function>(
      [&]() -> Function { return Function(definition, *signature); });
}

static auto validate_authored_function(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Functions require an authored addressable name."_view);
    return False;
  }

  if (definition.get_visibility() == Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Functions accept only `public` or `private` visibility."_view);
    return False;
  }

  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Functions do not accept evaluation modifiers."_view);
    return False;
  }

  // Attributes remain ordered source facts until an actual consumer asks for
  // one of their keys. Function therefore validates only its own grammar and
  // cannot constrain compiler, target, or embedding language extensions.
  return True;
}

auto Language::Function::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Function&> {
  Allocator::Arena& domain = cursor.get_arena();
  BAIL_IF(!validate_authored_function(cursor, definition));
  // Definition host is the Function's exact Type context and access authority.
  // No concrete member inventory participates in Function semantics.
  BAIL_IF(!definition.get_host().is<Language::Model::Type>());

  BAIL_IF(!cursor.require(
      Code::Type::Func,
      "Library Function definitions require the `func` qualifier."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Function qualifiers require `=` before their signature."_view));

  auto parsed_signature = Signature::interpret(cursor, definition.get_host());
  BAIL_IF(!parsed_signature);

  Function& function = domain.construct_from<Function>(
      [&]() -> Function { return Function(definition, *parsed_signature); });
  auto parsed_body =
      Flow::Block::interpret(cursor, function, function, function.get_host());
  BAIL_IF(!parsed_body);
  BAIL_IF(!function.definition.complete(
      definition.get_qualifier(),
      parsed_body->get_anchor().get_span().get_end()));

  function.body = *parsed_body;
  return function;
}

Language::Function::Function(
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature)
    : definition(definition), signature(signature) {}

auto Language::Function::link_declaration_signature(Cursor& cursor) -> Bool {
  if (is_signature_linked()) {
    return True;
  }

  // Signature routes receive the host Type directly. They therefore use the
  // same access authority as the body without making an incomplete Function
  // double as a Type resolution mode switch.
  return signature.link(cursor);
}

auto Language::Function::link_restored_declaration_signature() -> Bool {
  return signature.link_restored();
}

auto Language::Function::link_declaration_body(Cursor& cursor) -> Bool {
  BAIL_IF(!is_signature_linked());
  BAIL_IF(!body);

  // Signature edges publish before Block linking so every Identifier can reach
  // the exact Parameter object created for its authored declaration.
  BAIL_IF(!body->link(cursor));
  if (!get_results().is_empty() && !get_self_result() &&
      body->reaches_next_statement()) {
    cursor.create_expression_error(
        body->get_anchor(),
        "Function result Layout requires a terminal return statement."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    return False;
  }

  return True;
}

auto Language::Function::finalize_declaration(Cursor& cursor) -> Bool {
  BAIL_IF(!body);

  Bool valid = True;
  if (get_definition().is_published()) {
    valid = signature.validate_publication(cursor);
  }

  // Optional folding records a cached Constant for later consumers. A dynamic
  // result or failure remains queryable but cannot turn an otherwise complete
  // Function into a semantic failure without a Constant requirement.
  body->finalize(cursor);

  return valid;
}

auto Language::Function::resolve() const -> const Abstract& {
  if (!is_signature_linked()) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Function::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& parameter = signature.get_parameters().resolve_named(route);
  if (&parameter != &Invalid::get_invalid()) {
    return parameter;
  }

  return get_host().resolve_lexical_context(route);
}

auto Language::Function::get_parameters() const -> const Layout& {
  // Callable Layouts are total only after resolve() proves this Function's
  // Signature. Returning an empty Layout here would launder incomplete state
  // into valid zero value flow, so the lifecycle precondition remains explicit.
  return signature.get_parameters();
}

auto Language::Function::get_results() const -> const Layout& {
  return signature.get_results();
}

auto Language::Function::declares_self() const -> Bool {
  return signature.declares_self();
}

auto Language::Function::get_body() const -> Option<const Flow::Block&> {
  return body.visit(
      []() -> Option<const Flow::Block&> { return {}; },
      [](const Flow::Block& selected) -> Option<const Flow::Block&> {
        return selected;
      });
}

auto Language::Function::is_signature_linked() const -> Bool {
  return signature.is_linked();
}
