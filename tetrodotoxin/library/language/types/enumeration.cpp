// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/enumeration.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/builtin/enum/name.hpp"
#include "tetrodotoxin/library/builtin/enum/size.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

static auto select_intrinsic_type(
    const Types::Enumeration& enumeration,
    Core::View::Bytes name) -> Option<const Model::Type&> {
  return enumeration.get_host()
      .resolve_context(name)
      .resolve()
      .select<Model::Type>();
}

static auto select_name_type(const Types::Enumeration& enumeration)
    -> Option<const Model::Type&> {
  auto bytes = select_intrinsic_type(enumeration, "Unsigned_8"_view);
  const Abstract& selected =
      enumeration.get_host().resolve_context("View"_view).resolve();
  auto generic = selected.select<Generic>();
  BAIL_IF(!bytes || !generic);

  Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(*bytes),
  }};
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Model::Type& type) -> Option<const Model::Type&> {
            return type;
          },
          [](const Generic::Failure&) -> Option<const Model::Type&> {
            return {};
          });
}

struct ParsedCase {
  Core::View::Bytes name;
  Core::View::Bytes value;
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
  Core::View::Bytes source = cursor.get_source_text();
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
    Core::View::Bytes text,
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
    Core::View::Bytes text,
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
  auto selected_type = selected->select<Model::Type>();
  Bool integer = selected_type && (selected_type->is<Model::Types::Signed>() ||
                                   selected_type->is<Model::Types::Unsigned>());
  if (!integer || !selected_type) {
    cursor.create_expression_error(
        storage_reference.get_anchor(),
        "Enumeration storage did not resolve to an exact integer Type."_view,
        "Select one concrete Library Signed or Unsigned Type."_view);
    return False;
  }

  auto count_type = select_intrinsic_type(*this, "Unsigned_64"_view);
  auto unsigned_count = count_type
                            ? count_type->select<Model::Types::Unsigned>()
                            : Option<const Model::Types::Unsigned&>();
  auto name_type = select_name_type(*this);
  if (!unsigned_count || !name_type) {
    cursor.create_expression_error(
        get_anchor(),
        "Enumeration could not materialize its generated Callable Types."_view,
        "Keep Unsigned_8, Unsigned_64, and View available in the Library root."_view);
    return False;
  }

  publish_callable(
      domain, Builtin::Enum::Name::create(domain, *this, *name_type), True);
  auto& size = Builtin::Enum::Size::create(
      domain, *unsigned_count, source_cases.get_size());
  generated_size = Option<Reference<const Model::Addressable>>(
      Reference<const Model::Addressable>(size));

  storage_type = Reference<const Model::Type>(*selected_type);
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

  const Model::Type& type = storage_type->get();
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
    Unsigned_64 representation = type.is<Model::Types::Signed>()
                                     ? Unsigned_64(value.signed_value)
                                     : value.unsigned_value;
    const Abstract& constant = Constants::Enumeration::create_authored(
        domain, *this, representation, source_case.value_anchor);
    const Ttx::Model::Alias& alias = domain.construct<Ttx::Model::Alias>(
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
    Core::View::Bytes route) const -> const Abstract& {
  if (stage != Stage::Finalized) {
    return Invalid::get_invalid();
  }

  auto case_view = cases.get_view();
  for (Count i = 0; i < cases.get_size(); i++) {
    const Ttx::Model::Alias& alias = case_view.get_data()[i].get();
    if (alias.get_name() == route) {
      return alias;
    }
  }

  return Invalid::get_invalid();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::resolve_type_access(
    const Abstract&,
    Core::View::Bytes route,
    Model::Type::Access access) const -> const Abstract& {
  if (access == Model::Type::Access::Static && generated_size &&
      generated_size->get().get_name() == route) {
    return generated_size->get();
  }

  return Invalid::get_invalid();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  BAIL_IF(!storage_type);
  return Constants::Enumeration::create_synthetic(arena, *this, 0);
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_storage_type()
    const -> Option<const Model::Type&> {
  return storage_type.visit(
      []() -> Option<const Model::Type&> { return {}; },
      [](const Reference<const Model::Type>& selected)
          -> Option<const Model::Type&> { return selected.get(); });
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_cases() const
    -> Core::View::Vector<Reference<const Ttx::Model::Alias>> {
  return cases;
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_case_value(
    Count index) const -> Option<Unsigned_64> {
  if (index >= cases.get_size()) {
    return {};
  }

  auto value = cases.get_view()
                   .get_data()[index]
                   .get()
                   .resolve()
                   .select<Constants::Enumeration>();
  return value ? Option<Unsigned_64>(value->get_value())
               : Option<Unsigned_64>();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_case_name(
    Count index) const -> Core::View::Bytes {
  return index < cases.get_size()
             ? cases.get_view().get_data()[index].get().get_name()
             : Core::View::Bytes();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::find_case_name(
    Unsigned_64 value) const -> Core::View::Bytes {
  for (Count index = 0; index < cases.get_size(); index++) {
    auto candidate = get_case_value(index);
    if (candidate && *candidate == value) {
      return get_case_name(index);
    }
  }

  return {};
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::accepts_iteration(
    const Layout& bindings) const -> Bool {
  auto value_entry = bindings.get_abstract(0);
  auto value = value_entry ? value_entry->select<Ttx::Model::Addressable>()
                           : Option<const Ttx::Model::Addressable&>();
  auto value_name = bindings.get_name(0);
  if (!value || !value_name || *value_name != "value"_view) {
    return False;
  }

  if (bindings.get_size() == 1) {
    return &value->get_type().resolve() == &resolve();
  }

  if (bindings.get_size() != 2 || !storage_type ||
      &value->get_type().resolve() != &storage_type->get().resolve()) {
    return False;
  }

  auto name_entry = bindings.get_abstract(1);
  auto name = name_entry ? name_entry->select<Ttx::Model::Addressable>()
                         : Option<const Ttx::Model::Addressable&>();
  auto name_name = bindings.get_name(1);
  auto view = name ? name->get_type().resolve().select<Types::View>()
                   : Option<const Types::View&>();
  auto byte_type = select_intrinsic_type(*this, "Unsigned_8"_view);
  return name_name && *name_name == "name"_view && view && byte_type &&
         &view->get_element_type().resolve() == &byte_type->resolve();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::begin_iteration(
    Llvm::Builder& body,
    const Abstract& owner,
    const Layout& bindings,
    const Ttx::Model::Pack&) const -> Bool {
  BAIL_IF(!accepts_iteration(bindings));

  Dynamic::Vector<Unsigned_64> values;
  Dynamic::Vector<Core::View::Bytes> names;
  values.resize(cases.get_size());
  if (bindings.get_size() == 2) {
    names.resize(cases.get_size());
  }

  for (Count index = 0; index < cases.get_size(); index++) {
    auto value = get_case_value(index);
    BAIL_IF(!value);
    values[index] = *value;
    if (names.get_size()) {
      names[index] = get_case_name(index);
    }
  }

  return body.begin_enumeration(
      owner, bindings, values.get_view(), names.get_view());
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::reserve(
    Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto storage = get_storage_type();
  if (!storage) {
    return False;
  }

  auto reserved =
      carriers.reserve(program, *this, Llvm::Carriers::Kind::Enumeration);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool storage_reserved = storage->reserve(program);
  if (!storage_reserved) {
    return False;
  }

  if (!generated_size || !generated_size->get().reserve_declaration(program)) {
    return False;
  }

  return reserve_callables(program);
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::complete(
    Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto storage = get_storage_type();
  if (!storage) {
    return False;
  }

  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool storage_completed = storage->complete(program);
  if (!storage_completed) {
    return False;
  }

  if (!generated_size || !generated_size->get().complete_declaration(program)) {
    return False;
  }

  Bool callables_completed = complete_callables(program);
  if (!callables_completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Enumeration);
  if (!carrier_completed || !complete_debug(program)) {
    return False;
  }

  for (Count index = 0; index < cases.get_size(); index++) {
    const Ttx::Model::Alias& alias = cases.get_view().get_data()[index].get();
    auto value = get_case_value(index);
    if (!value) {
      return False;
    }

    if (storage->is<Model::Types::Signed>()) {
      if (!program.get_debug().signed_enumerator(
              program, *this, alias, Signed_64(*value))) {
        return False;
      }
    } else {
      if (!program.get_debug().unsigned_enumerator(
              program, *this, alias, *value)) {
        return False;
      }
    }
  }

  return True;
}
