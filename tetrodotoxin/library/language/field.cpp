// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/field.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Field::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Field);
  Archive::Declaration declaration(definition);
  BAIL_IF(!declaration.write(writer));

  writer.write(U8(writability));
  writer.write(U8(type_reference ? 1 : 0));
  BAIL_IF(type_reference && !type_reference->persist(writer));

  Bool include_constant = writability == Writability::Constant;
  auto folded = include_constant ? get_constant() : Option<Model::Pack&>();
  writer.write(U8(include_constant ? 1 : 0));
  BAIL_IF(
      include_constant &&
      (!folded || !Model::Pack::persist_folded(writer, *folded)));
  return writer.finish(record);
}

auto Language::Field::persist_slot(Archive::Writer& writer, Count ordinal) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::FieldSlot);
  writer.write(U64(ordinal));
  writer.write(U8(type_reference ? 1 : 0));
  BAIL_IF(type_reference && !type_reference->persist(writer));
  return writer.finish(record);
}

auto Language::Field::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Field&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Field) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto encoded_writability = contents.read_u8();
  auto has_type = contents.read_u8();
  BAIL_IF(
      !declaration || !encoded_writability ||
      *encoded_writability > U8(Writability::Constant) || !has_type ||
      *has_type > 1);

  Option<TypeReference> type_reference;
  if (*has_type == 1) {
    auto restored = TypeReference::restore(contents, arena, host);
    BAIL_IF(!restored);
    type_reference = *restored;
  }

  auto has_initializer = contents.read_u8();
  BAIL_IF(!has_initializer || *has_initializer > 1);
  Option<Model::Pack&> initializer;
  if (*has_initializer == 1) {
    initializer = Model::Pack::restore_folded(contents, arena, host);
    BAIL_IF(!initializer);
  }
  BAIL_IF(!contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return arena.construct_from<Field>([&]() -> Field {
    return Field(
        arena, definition, Writability(*encoded_writability), type_reference,
        initializer);
  });
}

auto Language::Field::restore_slot(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Count ordinal) -> Option<Field&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::FieldSlot) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto encoded_ordinal = contents.read_u64();
  auto has_type = contents.read_u8();
  BAIL_IF(
      !encoded_ordinal || *encoded_ordinal != ordinal || !has_type ||
      *has_type > 1);

  Option<TypeReference> type_reference;
  if (*has_type == 1) {
    auto restored = TypeReference::restore(contents, arena, host);
    BAIL_IF(!restored);
    type_reference = *restored;
  }

  BAIL_IF(!contents.is_complete());

  Managed::Bytes name(arena, "$slot"_view);
  Perimortem::Serialization::Stream::Textual<Managed::Bytes> stream(name);
  stream << ordinal;
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      arena, Documentation::get_empty(), host, name.get_view(),
      Tetrodotoxin::Language::Visibility::Private, Anchor::create(Span()));
  return arena.construct_from<Field>([&]() -> Field {
    return Field(
        arena, definition, Writability::Internal, type_reference,
        Option<Model::Pack&>());
  });
}

static auto parse_writability(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> Option<Language::Writability> {
  auto modifiers = definition.get_modifiers();
  if (modifiers.get_size() > 1) {
    cursor.create_token_error(
        modifiers.get_data()[1],
        "Library Fields accept at most one evaluation modifier."_view);
    return {};
  }

  Language::Writability writability = Language::Writability::Full;
  if (!modifiers.is_empty()) {
    switch (modifiers.get_data()[0].get_code().get_type()) {
    case Code::Type::State:
      writability = Language::Writability::Internal;
      break;
    case Code::Type::Const:
      writability = Language::Writability::Constant;
      break;
    default:
      cursor.create_token_error(
          modifiers.get_data()[0],
          "Library Fields accept only `state` or `const` evaluation."_view);
      return {};
    }
  }

  Tetrodotoxin::Language::Visibility visibility = definition.get_visibility();
  if (visibility == Tetrodotoxin::Language::Visibility::Exposed &&
      writability != Language::Writability::Internal) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return {};
  }
  return writability;
}

auto Language::Field::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Field&> {
  Allocator::Arena& domain = cursor.get_arena();
  // Definition proves the exact Library Type that supplies declaration
  // context and access authority. Field does not require one concrete host.
  BAIL_IF(!definition.get_host().is<Language::Model::Type>());
  auto writability = parse_writability(definition, cursor);
  BAIL_IF(!writability);

  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Fields require an addressable name."_view);
    return {};
  }

  Option<TypeReference> type;
  Option<Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    if (Expressions::Initializer::is_next(cursor)) {
      auto object_initializer =
          Expressions::Initializer::parse(definition.get_host(), cursor);
      BAIL_IF(!object_initializer);
      initializer = *object_initializer;
    } else {
      initializer = Model::Parser::Pack::parse(definition.get_host(), cursor);
    }
    BAIL_IF(!initializer);
  } else {
    auto authored_type = TypeReference::parse(definition.get_host(), cursor);
    BAIL_IF(!authored_type);
    type = *authored_type;

    if (cursor.matches(Code::Type::Assign)) {
      cursor.consume();
      if (Expressions::Initializer::is_next(cursor)) {
        auto object_initializer =
            Expressions::Initializer::parse(definition.get_host(), cursor);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Model::Parser::Pack::parse(definition.get_host(), cursor);
      }
      BAIL_IF(!initializer);
    } else if (*writability == Writability::Constant) {
      cursor.create_token_error(
          "Library const Fields require an initializer."_view);
      return {};
    }
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  BAIL_IF(!terminator);

  BAIL_IF(!definition.complete(definition.get_name_token(), terminator));
  Field& field = domain.construct_from<Field>([&]() -> Field {
    return Field(domain, definition, *writability, type, initializer);
  });
  return field;
}

auto Language::Field::link_declaration_type(Cursor& cursor) -> Bool {
  if (!type_reference) {
    return True;
  }

  auto selected = type_reference->resolve_authored(cursor, *this);
  BAIL_IF(!selected);
  auto selected_type = selected->select<Language::Model::Type>();
  if (!selected_type) {
    cursor.create_expression_error(
        get_type_anchor(),
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return False;
  }

  if (selected_type->get_layout().is_empty()) {
    cursor.create_expression_error(
        get_type_anchor(), "Field cannot bind an empty Type Layout."_view,
        "Use the empty Type as a Static namespace or choose a Type with one "
        "value leaf."_view);
    return False;
  }

  if (type) {
    if (&type->get() == &*selected_type) {
      return True;
    }

    cursor.create_expression_error(
        get_type_anchor(),
        "Field Type linking selected a different semantic identity."_view,
        "Repeat completion with the same resolved Type edge."_view);
    return False;
  }

  type = Reference<const Language::Model::Type>(*selected_type);
  return True;
}

auto Language::Field::link_restored_declaration_type() -> Bool {
  if (!type_reference) {
    return True;
  }

  Option<const Model::Type&> selected_type;
  type_reference->resolve_lexical(*this).visit(
      [&](const Abstract& selected) {
        selected_type = selected.select<Model::Type>();
      },
      [](const TypeReference::Failure&) {});
  BAIL_IF(!selected_type || selected_type->get_layout().is_empty());
  type = Reference<const Model::Type>(*selected_type);
  return True;
}

auto Language::Field::link_restored_declaration_initializer() -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Option<Model::Pack&> { return selected; });
  BAIL_IF(!selected_initializer);
  BAIL_IF(!selected_initializer->link_restored(*this, get_host()));
  BAIL_IF(&selected_initializer->resolve() != &*selected_initializer);

  if (!type) {
    BAIL_IF(selected_initializer->get_layout().get_size() != 1);
    const Abstract& output = selected_initializer->get_type();
    const Abstract& resolved =
        output.is<Model::Type>() ? output : output.resolve();
    auto inferred = resolved.select<Model::Type>();
    BAIL_IF(!inferred || inferred->get_layout().is_empty());
    type = Reference<const Model::Type>(*inferred);
  } else {
    BAIL_IF(!selected_initializer->fits_into(type->get()));
  }

  initializer_linked = True;
  return writability != Writability::Constant || cache_constant();
}

auto Language::Field::link_inferred_declaration_type(Cursor& cursor) -> Bool {
  if (type_reference) {
    return True;
  }

  return link_declaration_initializer(cursor);
}

auto Language::Field::link_declaration_initializer(Cursor& cursor) -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Option<Model::Pack&> { return selected; });
  if (!selected_initializer) {
    cursor.create_expression_error(
        get_anchor(), "Field initializer state is incomplete."_view,
        "Retain one initializer before linking restricted Field access."_view);
    return False;
  }

  // The host retained this exact Field before linking and exposes only its
  // completed declaration prefix. That path authenticates the real host
  // without inventing another initializer context.
  // The Field remains the lexical owner of bare names. Its host Type travels
  // separately as access authority so nested expressions never have to infer
  // scope from the concrete declaration category.
  BAIL_IF(!selected_initializer->link(cursor, *this, get_host()));
  // Linking a Type name is valid when a later access consumes its identity.
  // Field is a value owner, so it proves real Pack flow before reading Layout.
  if (&selected_initializer->resolve() != &*selected_initializer) {
    cursor.create_expression_error(
        get_anchor(), "Field initializer did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }

  if (!type) {
    if (selected_initializer->get_layout().get_size() != 1) {
      auto report = cursor.create_report(get_anchor());
      report << "Cannot infer Field '"_view << get_name() << "' from "_view
             << selected_initializer->get_layout().get_size()
             << " initializer values.\nSource produces: "_view;
      Language::Diagnostics::write_pack(report, *selected_initializer);
      report.get_hint()
          << "Provide exactly one value or declare the Field's receiving Type."_view;
      return False;
    }

    const Abstract& result_type = selected_initializer->get_type();
    // A scalar Pack can expose one exact Type before that Type finishes its
    // Layout. Inference retains that real identity directly rather than
    // resolving it to the incomplete sentinel.
    const Abstract& resolved_type = result_type.is<Language::Model::Type>()
                                        ? result_type
                                        : result_type.resolve();
    auto initializer_type = resolved_type.select<Language::Model::Type>();
    if (!initializer_type) {
      cursor.create_expression_error(
          get_anchor(),
          "Inferred Field initializer did not complete one stable Type."_view,
          "Use an initializer whose exact Type settles before Field "
          "publication."_view);
      return False;
    }

    if (initializer_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          get_anchor(), "Inferred Field cannot bind an empty Type Layout."_view,
          "Keep the empty result as flow or infer from a Type with one value "
          "leaf."_view);
      return False;
    }

    type = Reference<const Language::Model::Type>(*initializer_type);
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits_into(type->get())) {
    auto report = cursor.create_report(get_anchor());
    report << "Initializer for Field '"_view << get_name()
           << "' does not fit declared Type '"_view << type->get().get_name()
           << "'.\nSource produces: "_view;
    Language::Diagnostics::write_pack(report, *selected_initializer);
    report << "\nTarget accepts: "_view;
    Language::Diagnostics::write_layout(report, type->get().get_layout());
    report.get_hint()
        << "Change the initializer or declare the exact Type it produces."_view;
    return False;
  }

  initializer_linked = True;
  return True;
}

auto Language::Field::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return get_host().resolve_lexical_context(route);
}

auto Language::Field::link_declaration_constant(Cursor& cursor) const -> Bool {
  if (writability != Writability::Constant || cache_constant()) {
    return True;
  }

  cursor.create_expression_error(
      get_anchor(),
      "Const Field initializer did not resolve to a compile-time value."_view,
      "Use only fully linked constant Expressions in a const Field."_view);
  return False;
}

auto Language::Field::validate_publication(Cursor& cursor) const -> Bool {
  if (!get_definition().is_published()) {
    return True;
  }

  const Model::Type& host = get_host();
  Bool reachable = type_reference.visit(
      [&]() { return host.is_externally_reachable(get_type()); },
      [&](const TypeReference& reference) {
        Option<const Abstract&> selected;
        reference.resolve(host).visit(
            [&](const Abstract& resolved) { selected = resolved; },
            [](const TypeReference::Failure&) {});
        return Bool(selected && &selected->resolve() == &get_type());
      });
  if (reachable) {
    return True;
  }

  cursor.create_expression_error(
      get_type_anchor().visit(
          [&]() -> Option<Anchor> { return get_anchor(); },
          [](Anchor selected) -> Option<Anchor> { return selected; }),
      "Externally readable Field publishes an unreachable Type."_view,
      "Keep the Field private or publish its exact Type."_view);
  return False;
}

auto Language::Field::finalize_declaration(Cursor& cursor) -> Bool {
  initializer.visit(
      []() {}, [&](Model::Pack& selected) { selected.finalize(cursor); });
  return validate_publication(cursor);
}

auto Language::Field::resolve() const -> const Abstract& {
  if (!type) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Field::get_initializer() const -> Option<const Model::Pack&> {
  return initializer.visit(
      []() -> Option<const Model::Pack&> { return {}; },
      [](const Model::Pack& selected) -> Option<const Model::Pack&> {
        return selected;
      });
}

auto Language::Field::get_constant() const -> Option<Model::Pack&> {
  if (writability != Writability::Constant || !cache_constant()) {
    return {};
  }

  return constant.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](const Reference<Model::Pack>& selected) -> Option<Model::Pack&> {
        return selected.get();
      });
}

auto Language::Field::cache_constant() const -> Bool {
  // Folding is reentrant through const references. The explicit state keeps a
  // cycle distinct from a dynamic value that may settle after another owner.
  if (constant_state == ConstantState::Folded) {
    return True;
  }

  if (constant_state == ConstantState::Folding ||
      constant_state == ConstantState::Failed) {
    return False;
  }

  constant_state = ConstantState::Folding;
  // The receiving Type gets first refusal because fitting may construct a new
  // semantic value such as an absent or present Option. Ordinary Types decline
  // that path and leave constant evaluation to the real source Expression.
  auto fitted = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [&](Model::Pack& source) {
        return get_type().create_fitted(domain, source);
      });
  if (fitted) {
    constant = Reference<Model::Pack>(*fitted);
    constant_state = ConstantState::Folded;
    return True;
  }

  auto expression = initializer.visit(
      []() -> Option<Expression&> { return {}; },
      [](Model::Pack& selected) { return selected.select<Expression>(); });
  if (!expression) {
    constant_state = ConstantState::Failed;
    return False;
  }

  expression->fold().visit(
      [&](const Option<Model::Pack&>& folded) {
        // Absence is not permanent failure. Another const dependency can finish
        // during this closure, after which the same Field may be attempted
        // again.
        if (!folded) {
          constant_state = ConstantState::Unresolved;
          return;
        }
        constant = Reference<Model::Pack>(*folded);
        constant_state = ConstantState::Folded;
      },
      [&](const Expression::Error&) {
        constant_state = ConstantState::Failed;
      });
  return constant_state == ConstantState::Folded;
}
