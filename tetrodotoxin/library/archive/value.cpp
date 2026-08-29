// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/value.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/object.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/result.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto write_constant(
    Archive::Writer& writer,
    const Language::Constant& constant) -> Bool {
  auto flag = constant.select<Language::Constants::Flag>();
  if (flag) {
    Archive::Tag tag = flag->get_value() ? Archive::Tag::ConstantTrue
                                         : Archive::Tag::ConstantFalse;
    auto record = writer.begin(tag);
    return writer.write(flag->get_type().get_name()) && writer.finish(record);
  }

  auto unsigned_value = constant.select<Language::Constants::Unsigned>();
  if (unsigned_value) {
    auto record = writer.begin(Archive::Tag::ConstantUnsigned);
    BAIL_IF(!writer.write(unsigned_value->get_type().get_name()));
    writer.write(unsigned_value->get_value());
    return writer.finish(record);
  }

  auto signed_value = constant.select<Language::Constants::Signed>();
  if (signed_value) {
    auto record = writer.begin(Archive::Tag::ConstantSigned);
    BAIL_IF(!writer.write(signed_value->get_type().get_name()));
    writer.write(signed_value->get_value());
    return writer.finish(record);
  }

  auto real = constant.select<Language::Constants::Real>();
  if (real) {
    auto record = writer.begin(Archive::Tag::ConstantReal);
    BAIL_IF(!writer.write(real->get_type().get_name()));
    writer.write(real->get_value());
    return writer.finish(record);
  }

  auto bytes = constant.select<Language::Constants::Bytes>();
  if (bytes) {
    auto resource = bytes->get_resource();
    auto record = writer.begin(
        resource ? Archive::Tag::ConstantResourceBytes
                 : Archive::Tag::ConstantBytes);
    return writer.write(bytes->get_type().get_name()) &&
           writer.write(resource ? resource->get_name() : bytes->get_value()) &&
           writer.finish(record);
  }

  auto object = constant.select<Language::Constants::Object>();
  if (object) {
    auto storage = object->get_type().select<Language::Types::ObjectStorage>();
    auto record = writer.begin(Archive::Tag::ConstantObject);
    BAIL_IF(
        !storage || !writer.write(object->get_type().get_name()) ||
        !writer.write(storage->get_element_type().get_name()));
    return writer.finish(record);
  }

  auto enumeration = constant.select<Language::Constants::Enumeration>();
  if (enumeration) {
    auto record = writer.begin(Archive::Tag::ConstantEnumeration);
    BAIL_IF(!writer.write(enumeration->get_type().get_name()));
    writer.write(enumeration->get_value());
    return writer.finish(record);
  }

  auto option = constant.select<Language::Constants::Option>();
  if (option) {
    auto record = writer.begin(Archive::Tag::ConstantOption);
    BAIL_IF(
        !writer.write(option->get_type().get_name()) ||
        !writer.write(option->get_type().get_element_type().get_name()));
    auto payload = option->get_payload();
    writer.write(U8(payload ? 1 : 0));
    BAIL_IF(
        (payload && !Archive::write_folded(writer, *payload)) ||
        !writer.finish(record));
    return True;
  }

  auto result = constant.select<Language::Constants::Result>();
  if (result) {
    auto record = writer.begin(Archive::Tag::ConstantResult);
    BAIL_IF(
        !writer.write(result->get_type().get_name()) ||
        !writer.write(result->get_type().get_value_type().get_name()) ||
        !writer.write(result->get_type().get_error_type().get_name()));
    writer.write(U8(result->get_kind()));
    return Archive::write_folded(writer, result->get_payload()) &&
           writer.finish(record);
  }

  auto range = constant.select<Language::Constants::Range>();
  if (range) {
    auto record = writer.begin(Archive::Tag::ConstantRange);
    return writer.write(range->get_type().get_name()) &&
           writer.write(range->get_type().get_element_type().get_name()) &&
           writer.finish(record);
  }

  return False;
}

auto Archive::write_folded(Writer& writer, const Language::Model::Pack& value)
    -> Bool {
  auto constant = value.select_identity<Language::Constant>();
  if (constant) {
    return write_constant(writer, *constant);
  }

  const Layout& layout = value.get_layout();
  BAIL_IF(layout.is_empty() || layout.get_size() > U32(-1));

  Bool named = Bool(layout.get_name(0));
  auto record = writer.begin(Tag::PackGroup);
  writer.write(U32(layout.get_size()));
  writer.write(U32(named ? layout.get_size() : 0));
  for (Count index = 0; named && index < layout.get_size(); index++) {
    auto name = layout.get_name(index);
    BAIL_IF(!name || !writer.write(*name));
  }

  for (Count index = 0; index < layout.get_size(); index++) {
    Bool selected_named = Bool(layout.get_name(index));
    auto producer = layout.get_abstract(index);
    auto selected = producer ? producer->select<Language::Constant>()
                             : Core::Option<const Language::Constant&>();
    BAIL_IF(
        selected_named != named || !selected ||
        !write_constant(writer, *selected));
  }
  return writer.finish(record);
}

static auto resolve_type(const Abstract& context, Core::View::Bytes name)
    -> Core::Option<const Language::Model::Type&> {
  return context.resolve_concept(name)
      .resolve()
      .select<Language::Model::Type>();
}

static auto materialize_type(
    const Abstract& context,
    Core::View::Bytes formula,
    Core::View::Vector<Language::Generic::Argument> arguments)
    -> Core::Option<const Language::Model::Type&> {
  auto generic =
      context.resolve_concept(formula).resolve().select<Language::Generic>();
  BAIL_IF(!generic);
  return generic->materialize(arguments).visit(
      [](const Language::Model::Type& selected)
          -> Core::Option<const Language::Model::Type&> { return selected; },
      [](const Language::Generic::Failure&)
          -> Core::Option<const Language::Model::Type&> { return {}; });
}

static auto restore_bytes_type(const Abstract& context, Count extent)
    -> Core::Option<const Language::Model::Type&> {
  auto element = resolve_type(context, "U8"_view);
  BAIL_IF(!element);

  if (extent == 0) {
    auto view = context.resolve_concept("View"_view)
                    .resolve()
                    .select<Language::Generic>();
    BAIL_IF(!view);
    Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
      Language::Generic::Argument(*element),
    }};
    return view->materialize(arguments.get_view())
        .visit(
            [](const Language::Model::Type& selected)
                -> Core::Option<const Language::Model::Type&> {
              return selected;
            },
            [](const Language::Generic::Failure&)
                -> Core::Option<const Language::Model::Type&> { return {}; });
  }

  auto fixed = context.resolve_concept("Fixed"_view)
                   .resolve()
                   .select<Language::Generic>();
  BAIL_IF(!fixed);

  Core::Static::Vector<Language::Generic::Argument, 2> arguments = {{
    Language::Generic::Argument(*element),
    Language::Generic::Argument(U64(extent)),
  }};
  return fixed->materialize(arguments.get_view())
      .visit(
          [](const Language::Model::Type& selected)
              -> Core::Option<const Language::Model::Type&> {
            return selected;
          },
          [](const Language::Generic::Failure&)
              -> Core::Option<const Language::Model::Type&> { return {}; });
}

auto Archive::read_folded(
    Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& lexical_context) -> Core::Option<Language::Model::Pack&> {
  auto record = reader.read_record();
  BAIL_IF(!record || record->is_optional());

  Reader contents(record->get_payload());
  Tag tag = Tag(record->get_tag());
  switch (tag) {
  case Tag::PackGroup: {
    auto count = contents.read_u32();
    auto name_count = contents.read_u32();
    BAIL_IF(
        !count || *count == 0 || !name_count ||
        (*name_count != 0 && *name_count != *count));

    Memory::Managed::Vector<Core::View::Bytes> names(arena);
    for (Count index = 0; index < *name_count; index++) {
      auto name = contents.read_bytes();
      BAIL_IF(!name);
      names.insert(arena.proxy(*name));
    }

    Memory::Managed::Vector<Ttx::Model::PackReference<Language::Model::Pack>>
        entries(arena);
    for (Count index = 0; index < *count; index++) {
      auto entry = read_folded(contents, arena, lexical_context);
      BAIL_IF(!entry);
      entries.insert(*entry);
    }
    BAIL_IF(!contents.is_complete());
    return Language::Model::Pack::create_completed(
        arena, entries.get_view(), names.get_view());
  }
  case Tag::ConstantFalse:
  case Tag::ConstantTrue: {
    auto type_name = contents.read_bytes();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto flag = type ? type->select<Language::Model::Types::Flag>()
                     : Core::Option<const Language::Model::Types::Flag&>();
    BAIL_IF(!flag || !contents.is_complete());
    return tag == Tag::ConstantTrue
               ? static_cast<Language::Model::Pack&>(
                     Language::Constants::True::create_synthetic(arena, *flag))
               : static_cast<Language::Model::Pack&>(
                     Language::Constants::False::create_synthetic(
                         arena, *flag));
  }
  case Tag::ConstantUnsigned: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_u64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected =
        type ? type->select<Language::Model::Types::Unsigned>()
             : Core::Option<const Language::Model::Types::Unsigned&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Unsigned::create_synthetic(
        arena, *selected, *value);
  }
  case Tag::ConstantSigned: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_s64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type
                        ? type->select<Language::Model::Types::Signed>()
                        : Core::Option<const Language::Model::Types::Signed&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Signed::create_synthetic(
        arena, *selected, *value);
  }
  case Tag::ConstantReal: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_r64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type ? type->select<Language::Model::Types::Real>()
                         : Core::Option<const Language::Model::Types::Real&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Real::create_synthetic(
        arena, *selected, *value);
  }
  case Tag::ConstantBytes: {
    auto ignored_type_name = contents.read_bytes();
    auto value = contents.read_bytes();
    BAIL_IF(!ignored_type_name || !value || !contents.is_complete());
    auto type = restore_bytes_type(lexical_context, value->get_size());
    BAIL_IF(!type);
    return Language::Constants::Bytes::create_synthetic(
        arena, *type, arena.proxy(*value));
  }
  case Tag::ConstantResourceBytes: {
    auto ignored_type_name = contents.read_bytes();
    auto route = contents.read_bytes();
    BAIL_IF(!ignored_type_name || !route || !contents.is_complete());
    Memory::Managed::Bytes complete_route(arena, "$["_view);
    complete_route.concat(*route);
    complete_route.append(']');
    auto resource = lexical_context.resolve_concept(complete_route.get_view())
                        .resolve()
                        .select<Tetrodotoxin::Language::Resource>();
    BAIL_IF(!resource);
    auto type =
        restore_bytes_type(lexical_context, resource->get_value().get_size());
    BAIL_IF(!type);
    return Language::Constants::Bytes::create_synthetic(
        arena, *type, resource->get_value(), *resource);
  }
  case Tag::ConstantObject: {
    auto ignored_type_name = contents.read_bytes();
    auto element_name = contents.read_bytes();
    auto element = element_name ? resolve_type(lexical_context, *element_name)
                                : Core::Option<const Language::Model::Type&>();
    BAIL_IF(!ignored_type_name || !element || !contents.is_complete());
    Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
      Language::Generic::Argument(*element),
    }};
    auto type =
        materialize_type(lexical_context, "Object"_view, arguments.get_view());
    BAIL_IF(!type || !type->is<Language::Types::ObjectStorage>());
    return Language::Constants::Object::create(arena, *type);
  }
  case Tag::ConstantEnumeration: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_u64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type ? type->select<Language::Types::Enumeration>()
                         : Core::Option<const Language::Types::Enumeration&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Enumeration::create_synthetic(
        arena, *selected, *value);
  }
  case Tag::ConstantOption: {
    auto ignored_type_name = contents.read_bytes();
    auto element_name = contents.read_bytes();
    auto present = contents.read_u8();
    auto element = element_name ? resolve_type(lexical_context, *element_name)
                                : Core::Option<const Language::Model::Type&>();
    BAIL_IF(!ignored_type_name || !element || !present || *present > 1);
    Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
      Language::Generic::Argument(*element),
    }};
    auto type =
        materialize_type(lexical_context, "Option"_view, arguments.get_view());
    auto selected = type ? type->select<Language::Types::Option>()
                         : Core::Option<const Language::Types::Option&>();
    BAIL_IF(!selected);
    if (*present == 0) {
      BAIL_IF(!contents.is_complete());
      return Language::Constants::Option::create_absent(arena, *selected);
    }

    auto payload = read_folded(contents, arena, lexical_context);
    BAIL_IF(!payload || !contents.is_complete());
    auto restored =
        Language::Constants::Option::create_present(arena, *selected, *payload);
    return restored ? Core::Option<Language::Model::Pack&>(*restored)
                    : Core::Option<Language::Model::Pack&>();
  }
  case Tag::ConstantResult: {
    auto ignored_type_name = contents.read_bytes();
    auto value_name = contents.read_bytes();
    auto error_name = contents.read_bytes();
    auto kind = contents.read_u8();
    auto value = value_name ? resolve_type(lexical_context, *value_name)
                            : Core::Option<const Language::Model::Type&>();
    auto error = error_name ? resolve_type(lexical_context, *error_name)
                            : Core::Option<const Language::Model::Type&>();
    BAIL_IF(
        !ignored_type_name || !value || !error || !kind ||
        *kind > U8(Language::Types::Result::Kind::Error));
    Core::Static::Vector<Language::Generic::Argument, 2> arguments = {{
      Language::Generic::Argument(*value),
      Language::Generic::Argument(*error),
    }};
    auto type =
        materialize_type(lexical_context, "Result"_view, arguments.get_view());
    auto selected = type ? type->select<Language::Types::Result>()
                         : Core::Option<const Language::Types::Result&>();
    auto payload = read_folded(contents, arena, lexical_context);
    BAIL_IF(!selected || !payload || !contents.is_complete());
    auto restored = *kind == U8(Language::Types::Result::Kind::Value)
                        ? Language::Constants::Result::create_value(
                              arena, *selected, *payload)
                        : Language::Constants::Result::create_error(
                              arena, *selected, *payload);
    return restored ? Core::Option<Language::Model::Pack&>(*restored)
                    : Core::Option<Language::Model::Pack&>();
  }
  case Tag::ConstantRange: {
    auto ignored_type_name = contents.read_bytes();
    auto element_name = contents.read_bytes();
    auto element = element_name ? resolve_type(lexical_context, *element_name)
                                : Core::Option<const Language::Model::Type&>();
    BAIL_IF(!ignored_type_name || !element || !contents.is_complete());
    Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
      Language::Generic::Argument(*element),
    }};
    auto type =
        materialize_type(lexical_context, "Range"_view, arguments.get_view());
    auto selected = type ? type->select<Language::Types::Range>()
                         : Core::Option<const Language::Types::Range&>();
    BAIL_IF(!selected);
    return Language::Constants::Range::create_synthetic(arena, *selected);
  }
  default:
    return {};
  }
}
