// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Structure::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Structure);
  Archive::Declaration declaration(get_definition());
  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Interface;
  BAIL_IF(
      !declaration.write(writer) ||
      !persist_declarations(writer, public_only) || !writer.finish(record));
  return True;
}

auto Types::Structure::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Structure&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != Unsigned_16(Archive::Tag::Structure) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  BAIL_IF(!declaration);

  auto& definition = declaration->create_definition(arena, host);
  Structure& structure = arena.construct_from<Structure>(
      [&]() -> Structure { return Structure(arena, definition, False); });
  BAIL_IF(!structure.restore_declarations(contents, profile));
  structure.complete_field_layout();
  return structure;
}

auto Types::Structure::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Structure&> {
  Allocator::Arena& domain = cursor.get_arena();
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Structure definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Structures accept only `public` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Structures do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Struct,
      "Library Structure definitions require the `struct` qualifier."_view);
  BAIL_IF(!kind_token);

  Structure& structure = domain.construct_from<Structure>(
      [&]() -> Structure { return Structure(domain, definition); });
  BAIL_IF(!structure.interpret_body(cursor, definition, kind_token));
  return structure;
}

auto Types::Structure::interpret_body(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Token kind_token) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Composite bodies require an opening `{`."_view));

  // The exact Type exists before shared body grammar so nested Functions retain
  // its final Arena identity. A rejected body leaves that private object
  // unreachable from the enclosing Composite.
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Composite body reached the end of source before `}`."_view);
      return False;
    }

    const Ttx::Concept::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto nested =
        Tetrodotoxin::Language::Definition::parse(cursor, documentation, *this);
    BAIL_IF(!nested || !interpret_definition(cursor, *nested));
  }

  Token closing = cursor.consume();
  BAIL_IF(!definition.complete(kind_token, closing));

  // Field identity and source order are complete with the authored body even
  // though each Field Type links later. Publishing that real Layout here makes
  // emptiness an immutable Type fact before Signatures negotiate their shape.
  complete_field_layout();
  return True;
}

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return Composite::resolve_context(route);
}

auto Types::Structure::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_layout().is_empty());

  if (construction && !provider_construction) {
    return construction->create_call(arena);
  }

  if (creating_default) {
    Perimortem::Core::Diagnostics::Log::Message<256> message(
        Perimortem::Core::Diagnostics::Log::Level::Error,
        Perimortem::Core::Diagnostics::Source());
    message << "Library default construction recursively reentered Type `"_view
            << get_name() << "`."_view;
    return {};
  }

  creating_default = True;
  auto result = [&]() -> Option<Model::Pack&> {
    // Structure owns this exact instance Field inventory, so selecting Field is
    // construction local to the owner rather than a consumer category switch.
    // Authored Layout order is filled from each Field initializer before asking
    // that Field's exact Type for its default.
    Managed::Vector<Ttx::Concept::Reference<Model::Pack>> values(arena);
    values.reset(get_layout().get_size());
    for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
         get_addressables()) {
      auto field = candidate.get().select<Field>();
      if (!field || field->get_writability() != Writability::Internal) {
        continue;
      }

      auto initializer = field->get_initializer();
      if (initializer) {
        values.insert(const_cast<Model::Pack&>(*initializer));
        continue;
      }

      auto value = field->get_type().create_default(arena);
      BAIL_IF(!value);
      values.insert(*value);
    }

    return Expressions::Initializer::create_synthetic(
        arena, *this, values.get_view());
  }();
  creating_default = False;
  return result;
}

auto Types::Structure::complete_construction() -> Bool {
  if (construction || get_layout().is_empty() ||
      !is_externally_reachable(*this)) {
    return True;
  }

  auto created =
      Construction::create(get_domain(), *this, provider_construction);
  BAIL_IF(!created);
  construction = *created;
  return True;
}

auto Types::Structure::link_fields(Cursor& cursor) -> Bool {
  return Composite::link_fields(cursor) && complete_construction();
}

auto Types::Structure::link_restored_fields() -> Bool {
  return Composite::link_restored_fields() && complete_construction();
}

auto Types::Structure::reserve(Llvm::Program& program) const -> Bool {
  return Composite::reserve(program) &&
         (!construction || construction->reserve_declaration(program));
}

auto Types::Structure::complete(Llvm::Program& program) const -> Bool {
  return Composite::complete(program) &&
         (!construction || construction->complete_declaration(program));
}

auto Types::Structure::lower(Llvm::Program& program) const -> Bool {
  return Composite::lower(program) &&
         (!construction || construction->lower_declaration(program));
}

auto Types::Structure::resolve_type_call(
    const Abstract& host,
    View::Bytes route,
    Model::Type::Access access) const -> const Abstract& {
  if (access == Model::Type::Access::Static && construction &&
      route == construction->get_name()) {
    return *construction;
  }

  return Composite::resolve_type_call(host, route, access);
}

auto Types::Structure::reserve_carrier(Llvm::Program& program) const
    -> Option<Bool> {
  const auto& carriers = program.get_carriers();
  Llvm::Carriers::Kind kind = get_layout().is_empty()
                                  ? Llvm::Carriers::Kind::Context
                                  : Llvm::Carriers::Kind::Structure;
  return carriers.reserve(program, *this, kind, get_definition());
}

auto Types::Structure::complete_carrier(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  Llvm::Carriers::Kind kind = get_layout().is_empty()
                                  ? Llvm::Carriers::Kind::Context
                                  : Llvm::Carriers::Kind::Structure;
  return carriers.complete(program, *this, kind);
}
