// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/enumeration.hpp"

#include "perimortem/core/math.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Alias;
using Type = Tetrodotoxin::Library::Language::Model::Type;

struct ParsedCase {
  View::Bytes name;
  View::Bytes value;
  const Documentation& documentation;
  Anchor anchor;
  Anchor name_anchor;
  Anchor value_anchor;
};

struct ParsedValue {
  Signed_64 signed_value;
  Unsigned_64 unsigned_value;
};

static auto parse_case(Cursor& cursor, const Documentation& documentation)
    -> Option<ParsedCase> {
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Library Enumeration cases require an addressable name."_view);
  BAIL_IF(!name_token);

  // TODO: Allow Enumeration cases to infer the next value when no assignment
  // is present.
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Enumeration cases require an explicit `=` value."_view));

  // TODO: Admit case expressions once complete folding can supply one
  // Constant.
  Token value_opening = cursor.current();
  Token value_token;
  if (cursor.matches(Code::Type::SubOp)) {
    cursor.consume();
    value_token = cursor.require(
        Code::Type::Numeric,
        "A negative Enumeration case requires a decimal integer."_view);
  } else if (
      cursor.matches(Code::Type::Numeric) || cursor.matches(Code::Type::Hex)) {
    value_token = cursor.consume();
  } else {
    cursor.create_token_error(
        "Library Enumeration cases require a decimal or hexadecimal "
        "integer."_view);
    return {};
  }

  BAIL_IF(!value_token);

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Enumeration cases require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Span value_span(value_opening, value_token);
  View::Bytes source = cursor.get_source_text();
  return ParsedCase{
    .name = name_token.caculate_text(source),
    .value = value_span.caculate_text(source),
    .documentation = documentation,
    .anchor = Anchor::create(name_token, Span(name_token, terminator)),
    .name_anchor = Anchor::create(Span(name_token)),
    .value_anchor = Anchor::create(value_token, value_span),
  };
}

static auto read_unsigned(
    View::Bytes text,
    Anchor anchor,
    Cursor& cursor,
    Unsigned_64& value) -> Bool {
  Bool hexadecimal = text.slice(0, 2) == "0x"_view;
  Reader::Textual reader(text.slice(hexadecimal ? 2 : 0));
  value = reader.read_unsigned(hexadecimal ? 16 : 10);
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        anchor,
        "Enumeration case exceeds the unsigned host integer domain."_view,
        "Use a complete integer representable by Unsigned_64."_view);
    return False;
  }

  return True;
}

static auto read_signed(
    View::Bytes text,
    Anchor anchor,
    Cursor& cursor,
    Signed_64& value) -> Bool {
  if (text.slice(0, 2) == "0x"_view) {
    Unsigned_64 unsigned_value = 0;
    Bool parsed = read_unsigned(text, anchor, cursor, unsigned_value);
    BAIL_IF(!parsed);
    if (unsigned_value > Unsigned_64(__INT64_MAX__)) {
      cursor.create_expression_error(
          anchor,
          "Enumeration hexadecimal case exceeds the signed host integer "
          "domain."_view,
          "Use a value no greater than Signed_64 maximum."_view);
      return False;
    }

    value = Signed_64(unsigned_value);
    return True;
  }

  Reader::Textual reader(text);
  value = reader.read_signed();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        anchor, "Enumeration case exceeds the signed host integer domain."_view,
        "Use a complete integer representable by Signed_64."_view);
    return False;
  }

  return True;
}

Tetrodotoxin::Library::Language::Types::Enumeration::Enumeration(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference storage_reference)
    : definition(definition),
      domain(domain),
      storage_reference(storage_reference),
      source_cases(domain),
      cases(domain) {}

auto Tetrodotoxin::Library::Language::Types::Enumeration::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Enumeration&> {
  // Cases remain local until the closing brace proves one complete body. A
  // malformed body rejects the source transaction without publishing a
  // partial case inventory.
  Allocator::Arena& domain = cursor.get_arena();
  // Enumeration needs only contextual queries from its Definition host. The
  // declaration owner independently decides where this Type is published.
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Enumerations require an authored Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Enumerations accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Enumerations do not accept evaluation modifiers."_view);
    return {};
  }

  Token enumeration_token = cursor.require(
      Code::Type::Enum,
      "Library Enumeration declarations require `enum`."_view);
  BAIL_IF(!enumeration_token);
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Library Enumeration storage requires an opening `[`."_view));

  auto storage = TypeReference::parse(definition.get_host(), cursor);
  BAIL_IF(!storage);
  BAIL_IF(!cursor.require(
      Code::Type::BracketEnd,
      "Library Enumeration storage requires a closing `]`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Enumeration bodies require an opening `{`."_view));

  Managed::Vector<ParsedCase> parsed_cases(domain);
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Enumeration body reached the end of source before "
          "`}`."_view);
      return {};
    }

    const Documentation& case_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto parsed = parse_case(cursor, case_documentation);
    BAIL_IF(!parsed);
    if (parsed_cases.get_view().contains([&](const ParsedCase& existing) {
          return existing.name == parsed->name;
        })) {
      cursor.create_expression_error(
          parsed->name_anchor,
          "Duplicate case name in one Library Enumeration."_view);
      return {};
    }

    parsed_cases.insert(*parsed);
  }

  Token closing = cursor.consume();
  BAIL_IF(!definition.complete(enumeration_token, closing));

  // The complete grammar begins one nonmoving Type and then copies only its
  // compact source slots. Constants and Aliases wait for the storage Type so
  // no provisional value graph survives interpretation.
  Enumeration& enumeration =
      domain.construct_from<Enumeration>([&]() -> Enumeration {
        return Enumeration(domain, definition, *storage);
      });
  enumeration.source_cases.reset(parsed_cases.get_size());
  for (Count i = 0; i < parsed_cases.get_size(); i++) {
    const ParsedCase& parsed = parsed_cases[i];
    SourceCase source_case{
      .name = parsed.name,
      .value = parsed.value,
      .documentation = parsed.documentation,
      .anchor = parsed.anchor,
      .name_anchor = parsed.name_anchor,
      .value_anchor = parsed.value_anchor,
    };
    enumeration.source_cases.insert(source_case);
  }

  return enumeration;
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::link_types(
    Cursor& cursor) -> Bool {
  if (stage >= Stage::StorageLinked) {
    return True;
  }

  auto selected = storage_reference.resolve_authored(cursor, get_host());
  BAIL_IF(!selected);
  auto selected_type = selected->select<Type>();
  Bool integer = selected_type && (selected_type->is<Model::Types::Signed>() ||
                                   selected_type->is<Model::Types::Unsigned>());
  if (!integer || !selected_type) {
    cursor.create_expression_error(
        storage_reference.get_anchor(),
        "Enumeration storage did not resolve to an exact integer Type."_view,
        "Select one concrete Library Signed or Unsigned Type."_view);
    return False;
  }

  storage_type = Reference<const Type>(*selected_type);
  stage = Stage::StorageLinked;
  return True;
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::finalize(
    Cursor& cursor) -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  if (stage != Stage::StorageLinked || !storage_type) {
    cursor.create_expression_error(
        get_anchor(),
        "An incomplete Enumeration cannot enter finalization."_view,
        "Link its exact integer storage Type before finalizing cases."_view);
    return False;
  }

  const Type& type = storage_type->get();
  Count storage_size =
      type.visit<Tetrodotoxin::Library::Language::Model::Types::Signed>(
          [](const Tetrodotoxin::Library::Language::Model::Types::Signed&
                 selected) { return selected.get_size(); },
          [](const Abstract& selected) {
            return static_cast<const Tetrodotoxin::Library::Language::Model::
                                   Types::Unsigned&>(selected)
                .get_size();
          });
  Managed::Vector<ParsedValue> values(domain);
  values.reset(source_cases.get_size());
  Bool failed = False;

  // Parsing and width checks finish for the complete inventory before any
  // Constant or Alias becomes queryable. One bad case therefore leaves the
  // Enumeration with no partial lookup surface.
  if (type.is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
    for (Count i = 0; i < source_cases.get_size(); i++) {
      const SourceCase& source_case = source_cases[i];
      Signed_64 value = 0;
      Bool parsed = read_signed(
          source_case.value, source_case.value_anchor, cursor, value);
      if (!parsed) {
        failed = True;
        continue;
      }

      if (!Math::is_representable(value, storage_size)) {
        cursor.create_expression_error(
            source_case.value_anchor,
            "Enumeration case does not fit its signed storage Type."_view,
            "Choose a value inside the selected byte width."_view);
        failed = True;
        continue;
      }

      values.insert(
          ParsedValue{
            .signed_value = value,
            .unsigned_value = 0,
          });
    }
  } else {
    for (Count i = 0; i < source_cases.get_size(); i++) {
      const SourceCase& source_case = source_cases[i];
      if (source_case.value[0] == '-') {
        cursor.create_expression_error(
            source_case.value_anchor,
            "Unsigned Enumeration storage cannot represent a negative "
            "case."_view,
            "Remove the leading minus or select an exact Signed Type."_view);
        failed = True;
        continue;
      }

      Unsigned_64 value = 0;
      Bool parsed = read_unsigned(
          source_case.value, source_case.value_anchor, cursor, value);
      if (!parsed) {
        failed = True;
        continue;
      }

      if (!Math::is_representable(value, storage_size)) {
        cursor.create_expression_error(
            source_case.value_anchor,
            "Enumeration case does not fit its unsigned storage Type."_view,
            "Choose a value inside the selected byte width."_view);
        failed = True;
        continue;
      }

      values.insert(
          ParsedValue{
            .signed_value = 0,
            .unsigned_value = value,
          });
    }
  }

  BAIL_IF(failed);

  cases.reset(source_cases.get_size());
  for (Count i = 0; i < source_cases.get_size(); i++) {
    const SourceCase& source_case = source_cases[i];
    const ParsedValue& value = values[i];
    const Abstract& constant = type.visit<
        Tetrodotoxin::Library::Language::Model::Types::Signed>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Signed&
                signed_type) -> const Abstract& {
          return Constants::Signed::create_authored(
              domain, signed_type, value.signed_value,
              source_case.value_anchor);
        },
        [&](const Abstract&) -> const Abstract& {
          const auto& unsigned_type = static_cast<
              const Tetrodotoxin::Library::Language::Model::Types::Unsigned&>(
              type);
          return Constants::Unsigned::create_authored(
              domain, unsigned_type, value.unsigned_value,
              source_case.value_anchor);
        });
    const Alias& alias = domain.construct<Alias>(
        source_case.name, constant, source_case.documentation);
    cases.insert(alias);
  }

  stage = Stage::Finalized;
  return True;
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::resolve() const
    -> const Abstract& {
  if (stage < Stage::StorageLinked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::resolve_context(
    View::Bytes route) const -> const Abstract& {
  if (stage != Stage::Finalized) {
    return Invalid::get_invalid();
  }

  auto case_view = cases.get_view();
  for (Count i = 0; i < cases.get_size(); i++) {
    const Alias& alias = case_view.get_data()[i].get();
    if (alias.get_name() == route) {
      return alias;
    }
  }

  return Invalid::get_invalid();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  BAIL_IF(!storage_type);
  return Constants::Enumeration::create_synthetic(arena, *this, 0);
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_storage_type()
    const -> Option<const Type&> {
  return storage_type.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Type>& selected) -> Option<const Type&> {
        return selected.get();
      });
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_cases() const
    -> View::Vector<Reference<const Alias>> {
  return cases;
}
