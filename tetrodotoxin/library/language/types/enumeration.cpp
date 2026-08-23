// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/enumeration.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/builtin/enum/name.hpp"
#include "tetrodotoxin/library/builtin/enum/size.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Enumeration::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Enumeration);
  Archive::Declaration declaration(definition);
  BAIL_IF(
      !declaration.write(writer) || !storage_reference.persist(writer) ||
      cases.get_size() > U32(-1));

  writer.write(U32(cases.get_size()));
  for (Count index = 0; index < cases.get_size(); index++) {
    auto case_record = writer.begin(Archive::Tag::EnumerationCase);
    const Ttx::Model::Alias& selected =
        cases.get_view().get_data()[index].get();
    auto value = get_case_value(index);
    BAIL_IF(
        !writer.write(selected.get_documentation()) ||
        !writer.write(selected.get_name()) || !value);
    writer.write(*value);
    BAIL_IF(!writer.finish(case_record));
  }
  return writer.finish(record);
}

auto Types::Enumeration::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Enumeration&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Enumeration) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto storage = TypeReference::restore(contents, arena, host);
  auto count = contents.read_u32();
  BAIL_IF(!declaration || !storage || !count);

  auto& definition = declaration->create_definition(arena, host);
  Enumeration& enumeration =
      arena.construct_from<Enumeration>([&]() -> Enumeration {
        return Enumeration(arena, definition, *storage);
      });
  enumeration.source_cases.reset(*count);
  enumeration.cases.reset(*count);
  for (Count index = 0; index < *count; index++) {
    auto case_record = contents.read_record();
    BAIL_IF(
        !case_record ||
        case_record->get_tag() != U16(Archive::Tag::EnumerationCase) ||
        case_record->is_optional());
    Archive::Reader case_contents(case_record->get_payload());
    auto documentation = case_contents.read_documentation(arena);
    auto name = case_contents.read_bytes();
    auto value = case_contents.read_u64();
    BAIL_IF(
        !documentation || !name || name->is_empty() || !value ||
        !case_contents.is_complete());

    Core::View::Bytes retained_name = arena.proxy(*name);
    enumeration.source_cases.insert(
        Case{
          .name = retained_name,
          .value = {},
          .documentation = *documentation,
          .anchor = Anchor::create(Span()),
          .name_anchor = Anchor::create(Span()),
          .value_anchor = Anchor::create(Span()),
        });
    const Abstract& constant =
        Constants::Enumeration::create_synthetic(arena, enumeration, *value);
    const Ttx::Model::Alias& alias = arena.construct<Ttx::Model::Alias>(
        retained_name, constant, *documentation);
    enumeration.cases.insert(alias);
  }
  BAIL_IF(!contents.is_complete());
  return enumeration;
}

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
  auto bytes = select_intrinsic_type(enumeration, "U8"_view);
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

struct ParsedValue {
  S64 signed_value;
  U64 unsigned_value;
};

static auto read_unsigned(
    Core::View::Bytes text,
    Anchor anchor,
    Cursor& cursor,
    U64& value) -> Bool {
  Bool hexadecimal = text.slice(0, 2) == "0x"_view;
  Reader::Textual reader(text.slice(hexadecimal ? 2 : 0));
  value = reader.read_unsigned(hexadecimal ? 16 : 10);
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        anchor,
        "Enumeration case exceeds the unsigned host integer domain."_view,
        "Use a complete integer representable by U64."_view);
    return False;
  }

  return True;
}

static auto read_signed(
    Core::View::Bytes text,
    Anchor anchor,
    Cursor& cursor,
    S64& value) -> Bool {
  if (text.slice(0, 2) == "0x"_view) {
    U64 unsigned_value = 0;
    Bool parsed = read_unsigned(text, anchor, cursor, unsigned_value);
    BAIL_IF(!parsed);
    if (unsigned_value > U64(__INT64_MAX__)) {
      cursor.create_expression_error(
          anchor,
          "Enumeration hexadecimal case exceeds the signed host integer "
          "domain."_view,
          "Use a value no greater than S64 maximum."_view);
      return False;
    }

    value = S64(unsigned_value);
    return True;
  }

  Reader::Textual reader(text);
  value = reader.read_signed();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        anchor, "Enumeration case exceeds the signed host integer domain."_view,
        "Use a complete integer representable by S64."_view);
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

auto Tetrodotoxin::Library::Language::Types::Enumeration::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference storage_reference,
    Core::View::Vector<Case> cases) -> Enumeration& {
  Enumeration& enumeration =
      domain.construct_from<Enumeration>([&]() -> Enumeration {
        return Enumeration(domain, definition, storage_reference);
      });
  enumeration.source_cases.reset(cases.get_size());
  for (const Case& source_case : cases) {
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

  auto count_type = select_intrinsic_type(*this, "U64"_view);
  auto unsigned_count = count_type
                            ? count_type->select<Model::Types::Unsigned>()
                            : Option<const Model::Types::Unsigned&>();
  auto name_type = select_name_type(*this);
  if (!unsigned_count || !name_type) {
    cursor.create_expression_error(
        get_anchor(),
        "Enumeration could not materialize its generated Callable Types."_view,
        "Keep U8, U64, and View available in the Library root."_view);
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

auto Types::Enumeration::link_restored_types() -> Bool {
  Option<const Model::Type&> selected_type;
  storage_reference.resolve_lexical(get_host())
      .visit(
          [&](const Abstract& selected) {
            selected_type = selected.select<Model::Type>();
          },
          [](const TypeReference::Failure&) {});
  BAIL_IF(
      !selected_type || (!selected_type->is<Model::Types::Signed>() &&
                         !selected_type->is<Model::Types::Unsigned>()));

  auto count_type = select_intrinsic_type(*this, "U64"_view);
  auto unsigned_count = count_type
                            ? count_type->select<Model::Types::Unsigned>()
                            : Option<const Model::Types::Unsigned&>();
  auto name_type = select_name_type(*this);
  BAIL_IF(!unsigned_count || !name_type);

  publish_callable(
      domain, Builtin::Enum::Name::create(domain, *this, *name_type), True);
  auto& size = Builtin::Enum::Size::create(
      domain, *unsigned_count, source_cases.get_size());
  generated_size = Reference<const Model::Addressable>(size);
  storage_type = Reference<const Model::Type>(*selected_type);
  stage = Stage::StorageLinked;
  return True;
}

auto Types::Enumeration::finalize_restored() -> Bool {
  BAIL_IF(
      stage != Stage::StorageLinked ||
      cases.get_size() != source_cases.get_size());
  stage = Stage::Finalized;
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
      const Case& source_case = source_cases[i];
      S64 value = 0;
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
      const Case& source_case = source_cases[i];
      if (source_case.value[0] == '-') {
        cursor.create_expression_error(
            source_case.value_anchor,
            "Unsigned Enumeration storage cannot represent a negative "
            "case."_view,
            "Remove the leading minus or select an exact Signed Type."_view);
        failed = True;
        continue;
      }

      U64 value = 0;
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
    const Case& source_case = source_cases[i];
    const ParsedValue& value = values[i];
    U64 representation = type.is<Model::Types::Signed>()
                             ? U64(value.signed_value)
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
    Count index) const -> Option<U64> {
  if (index >= cases.get_size()) {
    return {};
  }

  auto value = cases.get_view()
                   .get_data()[index]
                   .get()
                   .resolve()
                   .select<Constants::Enumeration>();
  return value ? Option<U64>(value->get_value()) : Option<U64>();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::get_case_name(
    Count index) const -> Core::View::Bytes {
  return index < cases.get_size()
             ? cases.get_view().get_data()[index].get().get_name()
             : Core::View::Bytes();
}

auto Tetrodotoxin::Library::Language::Types::Enumeration::find_case_name(
    U64 value) const -> Core::View::Bytes {
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
  auto byte_type = select_intrinsic_type(*this, "U8"_view);
  return name_name && *name_name == "name"_view && view && byte_type &&
         &view->get_element_type().resolve() == &byte_type->resolve();
}
