// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/parser/definition.hpp"
#include "tetrodotoxin/parser/documentation.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"

namespace Tetrodotoxin::Interpreter {

// Definitions evaluates the shared authored definition spine:
//
//   (Documentation)* [Publication] [Evaluation] (Type | Addressable) : Dialect
//
// Its template arguments are the complete compile time Dialect mapping for the
// enclosing source grammar. Definitions rejects every name that already
// resolves in the complete visible context, so nested definitions cannot shadow
// imports or outer definitions. It also owns mapping selection, rooting, and
// publication so Package, Group, and later definition bodies do not recreate
// collision policy or a runtime registry. The root owner and export surface may
// be the same object, as in Package, or distinct objects when a richer Dialect
// retains private definitions and publishes a narrower public graph.
//
// The owner retains every produced definition. Exports receives only the edge
// selected by the publication modifier. Visible contains borrowed outer
// contexts ordered from nearest to farthest. Keeping these roles separate lets
// a Library retain writable state while dependencies see only its chosen public
// adapter.
template <typename... definition_types>
class Definitions final {
 private:
  static consteval auto mapping_is_unique() -> Bool {
    const Perimortem::Core::View::Bytes names[] = {
      definition_types::get_name()...,
    };
    for (Count i = 0; i < sizeof...(definition_types); i++) {
      for (Count k = i + 1; k < sizeof...(definition_types); k++) {
        if (names[i] == names[k]) {
          return False;
        }
      }
    }

    return True;
  }

  static_assert(
      sizeof...(definition_types) > 0,
      "Definitions requires at least one compile-time mapping.");
  static_assert(
      mapping_is_unique(),
      "A Dialect name may appear only once in a Definitions mapping.");

 public:
  template <typename owner_type>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      owner_type& owner,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible = {})
      -> const Ttx::Concept::Abstract& {
    return evaluate(cursor, owner, owner, visible);
  }

  template <typename owner_type, typename exports_type>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      owner_type& owner,
      exports_type& exports,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract& {
    using namespace Perimortem::Core;
    using namespace Ttx::Concept;
    using namespace Ttx::Lexical;

    // Documentation and modifiers must be consumed before the name because
    // their authored order selects both publication and evaluation policy.
    const Ttx::Concept::Documentation& documentation =
        Documentation::evaluate(cursor);

    Static::Vector<Token, 2> modifier_storage;
    Count modifier_count = 0;
    if (cursor.current().get_code().is_publication_modifier()) {
      modifier_storage[modifier_count++] = cursor.consume();
    }
    if (cursor.current().get_code().is_evaluation_modifier()) {
      modifier_storage[modifier_count++] = cursor.consume();
    }

    const View::Vector<Token> modifiers(
        modifier_storage.get_data(), modifier_count);
    if (modifiers.is_empty()) {
      missing_modifier_error(cursor);
      return Invalid::get_invalid();
    }
    if (cursor.current().get_code().is_modifier()) {
      invalid_modifier_order_error(cursor, cursor.current());
      return Invalid::get_invalid();
    }
    if (modifiers[0].get_code() == Code::Type::Expose &&
        (modifiers.get_size() != 2 ||
         modifiers[1].get_code() != Code::Type::State)) {
      invalid_expose_prefix_error(cursor, modifiers[0]);
      return Invalid::get_invalid();
    }
    if (!cursor.matches(Code::Type::Type) &&
        !cursor.matches(Code::Type::Addressable)) {
      invalid_name_error(cursor, cursor.current());
      return Invalid::get_invalid();
    }

    const Token name = cursor.consume();
    const View::Bytes name_text = name.caculate_text(cursor.get_source_text());

    // Reject the name across the complete visible stack before asking a
    // Dialect to construct anything. This preserves the no shadowing rule and
    // keeps failed declarations out of every durable owner.
    if (!resolve_visible(name_text, owner, exports, visible)
             .template is<Invalid>()) {
      duplicate_name_error(cursor, name);
      return Invalid::get_invalid();
    }

    const Token define = cursor.require(
        Code::Type::Define, "Expected `:` after definition name."_view);
    if (!define.is_valid()) {
      return Invalid::get_invalid();
    }

    const Token dialect = cursor.current();
    const View::Bytes dialect_text =
        dialect.caculate_text(cursor.get_source_text());
    Bool registered = ((definition_types::get_name() == dialect_text) || ...);
    if (!registered) {
      missing_dialect_error(cursor, dialect);
      return Invalid::get_invalid();
    }

    // Definition Dialects see the current owner first followed by inherited
    // contexts from nearest to farthest. The export surface is deliberately
    // absent because internal evaluation uses retained definitions rather than
    // their consumer facing projections.
    Perimortem::Memory::Managed::Vector<Reference<Abstract>> contexts(
        cursor.get_arena());
    contexts.insert(Reference<Abstract>(owner));
    for (Count i = 0; i < visible.get_size(); i++) {
      contexts.insert(visible[i]);
    }

    const Abstract& definition = evaluate_selected<definition_types...>(
        cursor, modifiers, name, dialect, documentation, contexts.get_view());
    if (definition.is<Invalid>()) {
      return Invalid::get_invalid();
    }

    if (definition.get_name() != name_text) {
      invalid_result_error(cursor, dialect, name, definition.get_name());
      return Invalid::get_invalid();
    }

    // Expose publishes a stable read only view while retaining the Writable
    // definition internally. Validate that adapter before mutating either
    // destination so an invalid projection cannot enter the graph.
    const Code::Type first_modifier = modifiers[0].get_code().get_type();
    if (first_modifier == Code::Type::Expose) {
      if (!definition.is<Ttx::Model::Addressables::Writable>()) {
        invalid_expose_error(cursor, modifiers[0], definition);
        return Invalid::get_invalid();
      }

      if (&static_cast<const Abstract&>(owner) ==
          &static_cast<const Abstract&>(exports)) {
        invalid_expose_surface_error(cursor, modifiers[0]);
        return Invalid::get_invalid();
      }

      const Ttx::Model::Addressable& read_only =
          definition.assume<Ttx::Model::Addressables::Writable>()
              .get_read_only();
      if (read_only.is<Ttx::Model::Addressables::Writable>() ||
          read_only.get_name() != definition.get_name() ||
          &read_only.get_type().resolve() !=
              &definition.assume<Ttx::Model::Addressable>()
                   .get_type()
                   .resolve()) {
        invalid_read_only_error(cursor, modifiers[0], definition);
        return Invalid::get_invalid();
      }
    }

    // The producer has finished once it returns the definition. Definitions
    // now performs the owner transaction and publishes only the selected edge.
    Bool rooted = owner.add_root(definition);
    if (!rooted) {
      duplicate_name_error(cursor, name);
      return Invalid::get_invalid();
    }

    switch (first_modifier) {
    case Code::Type::Public: {
      Bool exported = exports.add_export(definition);
      if (!exported) {
        duplicate_name_error(cursor, name);
        return Invalid::get_invalid();
      }

      break;
    }
    case Code::Type::Expose: {
      const Ttx::Model::Addressable& read_only =
          definition.assume<Ttx::Model::Addressables::Writable>()
              .get_read_only();
      Bool exported = exports.add_export(read_only);
      if (!exported) {
        duplicate_name_error(cursor, name);
        return Invalid::get_invalid();
      }

      break;
    }
    default:
      break;
    }

    return definition;
  }

 private:
  template <typename owner_type, typename exports_type>
  static auto resolve_visible(
      Perimortem::Core::View::Bytes route,
      const owner_type& owner,
      const exports_type& exports,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract& {
    // Lookup proceeds from the current retained owner to its public adapter and
    // then through inherited contexts from nearest to farthest. The same order
    // governs both name collision checks and definition evaluation.
    const Ttx::Concept::Abstract& selected = owner.resolve_context(route);
    if (!selected.template is<Ttx::Concept::Invalid>()) {
      return selected;
    }

    const Ttx::Concept::Abstract& published = exports.resolve_context(route);
    if (!published.template is<Ttx::Concept::Invalid>()) {
      return published;
    }

    for (Count i = 0; i < visible.get_size(); i++) {
      const Ttx::Concept::Abstract& outer =
          visible[i].get().resolve_context(route);
      if (!outer.template is<Ttx::Concept::Invalid>()) {
        return outer;
      }
    }

    return Ttx::Concept::Invalid::get_invalid();
  }

  static auto invalid_name_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& found) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    const View::Bytes source = cursor.get_source_text();
    output << "Expected a Type or Addressable definition name after the "
              "modifier prefix, but found `"_view
           << found.caculate_text(source) << "`."_view;
    cursor.create_token_error(found, message);
  }

  static auto missing_modifier_error(Ttx::Lexical::Cursor& cursor) -> void {
    cursor.create_token_error(
        "Expected a publication or evaluation modifier before the definition "
        "name."_view);
  }

  static auto invalid_modifier_order_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& modifier) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Definition modifier `"_view
           << modifier.caculate_text(cursor.get_source_text())
           << "` is duplicated or out of order. Expected at most one "
              "publication modifier followed by at most one evaluation "
              "modifier."_view;
    cursor.create_token_error(modifier, message);
  }

  static auto invalid_expose_prefix_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& expose) -> void {
    cursor.create_token_error(
        expose, "`expose` is legal only as the `expose state` prefix."_view);
  }

  static auto duplicate_name_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& name) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Definition name `"_view
           << name.caculate_text(cursor.get_source_text())
           << "` already resolves in this context."_view;
    cursor.create_token_error(name, message);
  }

  static auto missing_dialect_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& dialect) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Definition Dialect `"_view
           << dialect.caculate_text(cursor.get_source_text())
           << "` is not registered. Expected one of {"_view;
    Count index = 0;
    ((output << (index++ == 0 ? "`"_view : ", `"_view)
             << definition_types::get_name() << "`"_view),
     ...);
    output << "}."_view;
    cursor.create_token_error(dialect, message);
  }

  static auto invalid_modifier_error(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers,
      const Ttx::Lexical::Token& dialect) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    const View::Bytes source = cursor.get_source_text();
    output << "Definition Dialect `"_view << dialect.caculate_text(source)
           << "` does not accept modifier prefix `"_view;
    for (Count i = 0; i < modifiers.get_size(); i++) {
      if (i != 0) {
        output << " "_view;
      }
      output << modifiers[i].caculate_text(source);
    }
    output << "`."_view;
    cursor.create_token_error(modifiers[0], message);
  }

  static auto invalid_expose_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& expose,
      const Ttx::Concept::Abstract& definition) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Definition `"_view << definition.get_name()
           << "` uses `expose` but does not prove Writable."_view;
    cursor.create_token_error(expose, message);
  }

  static auto invalid_expose_surface_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& expose) -> void {
    cursor.create_token_error(
        expose,
        "`expose state` requires distinct retained and exported definition "
        "surfaces."_view);
  }

  static auto invalid_read_only_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& expose,
      const Ttx::Concept::Abstract& definition) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    output << "Writable definition `"_view << definition.get_name()
           << "` did not supply an equivalent non-writable Addressable for "
              "`expose`."_view;
    cursor.create_token_error(expose, message);
  }

  static auto invalid_result_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& dialect,
      const Ttx::Lexical::Token& authored,
      Perimortem::Core::View::Bytes returned) -> void {
    using namespace Perimortem::Core;
    using namespace Perimortem::Memory;
    using namespace Perimortem::Serialization;

    Managed::Bytes message(cursor.get_arena());
    Stream::Textual<Managed::Bytes> output(message);
    const View::Bytes source = cursor.get_source_text();
    output << "Definition Dialect `"_view << dialect.caculate_text(source)
           << "` returned name `"_view << returned
           << "` for authored definition `"_view
           << authored.caculate_text(source) << "`."_view;
    cursor.create_token_error(authored, message);
  }

  template <typename selected_type, typename... remaining_types>
  static auto evaluate_selected(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers,
      const Ttx::Lexical::Token& name,
      const Ttx::Lexical::Token& dialect,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract& {
    using namespace Ttx::Concept;

    // The fixed mapping is expanded at compile time. Runtime work stops at the
    // one selected Dialect and never constructs a registry entry or callback.
    const Perimortem::Core::View::Bytes source = cursor.get_source_text();
    if (selected_type::get_name() == dialect.caculate_text(source)) {
      if (!selected_type::accepts(modifiers)) {
        invalid_modifier_error(cursor, modifiers, dialect);
        return Invalid::get_invalid();
      }

      return selected_type::template evaluate<Definitions<definition_types...>>(
          cursor, name.caculate_text(source), documentation, modifiers,
          visible);
    }

    if constexpr (sizeof...(remaining_types) > 0) {
      return evaluate_selected<remaining_types...>(
          cursor, modifiers, name, dialect, documentation, visible);
    }

    __builtin_trap();
  }
};

}  // namespace Tetrodotoxin::Interpreter
