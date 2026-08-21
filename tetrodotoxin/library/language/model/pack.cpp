// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/pack.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/object.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/result.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Model::Pack::link_restored(
    const Abstract&,
    Core::Option<const Abstract&>) -> Bool {
  return False;
}

class Group final : public Language::Model::Pack {
 public:
  class Layout final : public Ttx::Concept::Layout {
   public:
    struct Selection {
      Count entry;
      Count value;
    };

    constexpr Layout(const Group& group) : group(group) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override;
    auto get_name(Count index) const
        -> Core::Option<Core::View::Bytes> override;
    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source,
        Count target_index) const -> Bool override;
    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override;

    auto select(Count index) const -> Core::Option<Selection>;

   private:
    const Group& group;
  };

  Group(
      Memory::Allocator::Arena& domain,
      Core::View::Vector<Reference<Language::Model::Pack>> source_entries,
      Core::View::Vector<Core::View::Bytes> source_names,
      Core::Option<Ttx::Lexical::Anchor> anchor,
      Bool linked = False)
      : entries(domain),
        names(domain),
        anchor(anchor),
        layout(*this),
        linked(linked) {
    entries.reset(source_entries.get_size());
    for (const Reference<Language::Model::Pack>& entry : source_entries) {
      entries.insert(entry);
    }

    names.reset(source_names.get_size());
    for (Core::View::Bytes name : source_names) {
      names.insert(name);
    }
  }

  TTX_CONTRACT(Group, Language::Model::Pack);

  TTX_NAME("Pack"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Abstract& lexical_context,
      Core::Option<const Abstract&> access_scope) -> Bool override {
    Bool failed = False;
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      failed |= !entry.get().link(cursor, lexical_context, access_scope);
    }
    BAIL_IF(failed);

    // Type selection must link so a following access can query that identity.
    // A group is a value consumer, so it rejects the same result before Layout
    // observation turns the missing value output into a process failure.
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      if (&entry.get().resolve() != &entry.get()) {
        cursor.create_expression_error(
            anchor, "Library Pack entry did not produce value flow."_view,
            "Use a Type result only as an access receiver."_view);
        return False;
      }
    }

    if (!names.is_empty()) {
      for (Reference<Language::Model::Pack> entry : entries.get_view()) {
        if (entry.get().get_layout().get_size() != 1) {
          cursor.create_expression_error(
              anchor,
              "A named Library Pack entry must produce exactly one value."_view,
              "Name each scalar value separately or use positional flow."_view);
          return False;
        }
      }
    }

    linked = True;
    return True;
  }

  auto link_restored(
      const Abstract& lexical_context,
      Core::Option<const Abstract&> access_scope) -> Bool override {
    if (linked) {
      return True;
    }

    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      BAIL_IF(!entry.get().link_restored(lexical_context, access_scope));
      BAIL_IF(&entry.get().resolve() != &entry.get());
    }
    linked = True;
    return True;
  }

  auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }

  auto get_value_type(Count index) const -> const Abstract& override {
    auto selected = layout.select(index);
    if (!selected) {
      return Invalid::get_invalid();
    }
    return entries.at(selected->entry).get().get_value_type(selected->value);
  }

  auto get_produced(Count index) const
      -> Core::Option<Ttx::Model::Pack::Produced> override {
    auto selected = layout.select(index);
    BAIL_IF(!selected);
    return entries.at(selected->entry).get().get_produced(selected->value);
  }

  auto resolve() const -> const Abstract& override {
    return linked ? static_cast<const Language::Model::Pack&>(*this)
                  : static_cast<const Abstract&>(Invalid::get_invalid());
  }

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override {
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      entry.get().finalize(cursor);
    }
  }

  auto lower(Llvm::Builder& body) const -> Bool override {
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      if (!entry.get().lower(body)) {
        return False;
      }
    }

    return body.compose(*this);
  }

  Memory::Managed::Vector<Reference<Language::Model::Pack>> entries;
  Memory::Managed::Vector<Core::View::Bytes> names;
  Core::Option<Ttx::Lexical::Anchor> anchor;
  Layout layout;
  Bool linked = False;
};

auto Group::Layout::get_size() const -> Count {
  if (!group.names.is_empty()) {
    return group.entries.get_size();
  }

  Count size = 0;
  for (Reference<Language::Model::Pack> entry : group.entries.get_view()) {
    size += entry.get().get_layout().get_size();
  }
  return size;
}

auto Group::Layout::select(Count index) const -> Core::Option<Selection> {
  if (!group.names.is_empty()) {
    BAIL_IF(index >= group.entries.get_size());
    return Selection{index, 0};
  }

  Count offset = 0;
  for (Count entry = 0; entry < group.entries.get_size(); entry++) {
    Count size = group.entries.at(entry).get().get_layout().get_size();
    if (index < offset + size) {
      return Selection{entry, index - offset};
    }
    offset += size;
  }
  return {};
}

auto Group::Layout::get_abstract(Count index) const
    -> Core::Option<const Abstract&> {
  auto selected = select(index);
  BAIL_IF(!selected);
  return group.entries.at(selected->entry)
      .get()
      .get_layout()
      .get_abstract(selected->value);
}

auto Group::Layout::get_name(Count index) const
    -> Core::Option<Core::View::Bytes> {
  BAIL_IF(group.names.is_empty() || index >= group.names.get_size());
  return group.names.at(index);
}

static auto get_target_name(const Ttx::Concept::Layout& target, Count index)
    -> Core::Option<Core::View::Bytes> {
  auto name = target.get_name(index);
  if (name) {
    return name;
  }
  return target.get_abstract(index).visit(
      []() -> Core::Option<Core::View::Bytes> { return {}; },
      [](const Abstract& entry) -> Core::Option<Core::View::Bytes> {
        Core::View::Bytes name = entry.get_name();
        return name.is_empty() ? Core::Option<Core::View::Bytes>() : name;
      });
}

auto Group::Layout::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source,
    Count target_index) const -> Bool {
  BAIL_IF(source >= get_size() || target_index >= target.get_size());
  auto selected = select(source);
  BAIL_IF(!selected);

  if (!group.names.is_empty()) {
    auto source_name = get_name(source);
    auto target_name = get_target_name(target, target_index);
    BAIL_IF(!source_name || !target_name || *source_name != *target_name);
  }

  return group.entries.at(selected->entry)
      .get()
      .fits_entry(target, selected->value, target_index);
}

auto Group::Layout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  if (group.names.is_empty()) {
    for (Count source = 0; source < get_size(); source++) {
      BAIL_IF(!fits_entry(target, source, target_offset + source));
    }
    return True;
  }

  for (Count source = 0; source < get_size(); source++) {
    auto source_name = get_name(source);
    BAIL_IF(!source_name);

    Count selected = 0;
    Count matches = 0;
    for (Count candidate = 0; candidate < get_size(); candidate++) {
      auto target_name = get_target_name(target, target_offset + candidate);
      if (target_name && *target_name == *source_name) {
        selected = candidate;
        matches++;
      }
    }
    BAIL_IF(
        matches != 1 || !fits_entry(target, source, target_offset + selected));
  }
  return True;
}

auto Group::Layout::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> Utility::Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  Count source = target_index;
  if (!group.names.is_empty()) {
    auto target_name = get_target_name(target, target_offset + target_index);
    if (!target_name) {
      return Errors::IncompatibleFit;
    }
    for (Count candidate = 0; candidate < get_size(); candidate++) {
      auto source_name = get_name(candidate);
      if (source_name && *source_name == *target_name) {
        source = candidate;
        break;
      }
    }
  }

  auto selected = select(source);
  if (!selected) {
    return Errors::IncompatibleFit;
  }
  return group.entries.at(selected->entry)
      .get()
      .get_layout()
      .get_abstract(selected->value)
      .visit(
          []() -> Utility::Result<const Abstract&, Errors> {
            return Errors::IncompatibleFit;
          },
          [](const Abstract& entry)
              -> Utility::Result<const Abstract&, Errors> { return entry; });
}

auto Language::Model::Pack::get_type() const -> const Abstract& {
  const Ttx::Concept::Layout& layout = get_layout();
  if (layout.get_size() != 1) {
    return Invalid::get_invalid();
  }

  return get_value_type(0);
}

static auto select_target_type(const Abstract& target)
    -> Core::Option<const Language::Model::Type&> {
  auto direct = target.select<Language::Model::Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = target.resolve();
  auto addressable = resolved.select<Language::Model::Addressable>();
  const Abstract& selected = addressable ? addressable->get_type() : resolved;
  direct = selected.select<Language::Model::Type>();
  return direct ? direct : selected.resolve().select<Language::Model::Type>();
}

auto Language::Model::Pack::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  const Ttx::Concept::Layout& source = get_layout();
  BAIL_IF(
      source_index >= source.get_size() || target_index >= target.get_size());

  // The concrete Layout gets first refusal because it owns named selection,
  // ranged provenance, and any composed source mapping. A scalar producer is
  // the only fallback: its Library Pack contract can admit a contextual value
  // fit that raw Abstract identity cannot express.
  if (source.fits_entry(target, source_index, target_index)) {
    return True;
  }
  BAIL_IF(source.get_name(source_index));

  auto target_entry = target.get_abstract(target_index);
  BAIL_IF(!target_entry);
  auto target_type = select_target_type(*target_entry);
  BAIL_IF(!target_type);

  auto produced = get_produced(source_index);
  auto producer = produced ? produced->producer.select<Language::Model::Pack>()
                           : Core::Option<const Language::Model::Pack&>();
  return producer ? producer->fits_into(*target_type) : False;
}

auto Language::Model::Pack::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  const Ttx::Concept::Layout& source = get_layout();
  BAIL_IF(
      target_offset > target.get_size() ||
      source.get_size() > target.get_size() - target_offset);

  // A named Layout owns its target reordering. Its fits_entry implementation
  // still delegates each selected value back through Pack when necessary.
  for (Count index = 0; index < source.get_size(); index++) {
    if (source.get_name(index)) {
      return source.fits_at(target, target_offset);
    }
  }

  for (Count index = 0; index < source.get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Language::Model::Pack::fits(const Ttx::Concept::Layout& target) const
    -> Bool {
  BAIL_IF(&resolve() != this);
  if (get_layout().get_size() == target.get_size() && fits_at(target, 0)) {
    return True;
  }

  BAIL_IF(target.get_size() != 1);
  auto target_entry = target.get_abstract(0);
  BAIL_IF(!target_entry);
  auto target_type = select_target_type(*target_entry);
  return target_type && target_type->accepts(*this);
}

auto Language::Model::Pack::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const
    -> Utility::Result<const Abstract&, Ttx::Concept::Layout::Errors> {
  const Ttx::Concept::Layout& source = get_layout();
  if (target_index >= source.get_size()) {
    return Ttx::Concept::Layout::Errors::IndexOutOfBounds;
  }
  if (target_offset > target.get_size() ||
      source.get_size() > target.get_size() - target_offset) {
    return Ttx::Concept::Layout::Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Ttx::Concept::Layout::Errors::IncompatibleFit;
  }

  for (Count index = 0; index < source.get_size(); index++) {
    if (source.get_name(index)) {
      return source.get_fitted_at(target, target_offset, target_index);
    }
  }
  return source.get_abstract(target_index)
      .visit(
          []() -> Utility::Result<
                   const Abstract&, Ttx::Concept::Layout::Errors> {
            return Ttx::Concept::Layout::Errors::IncompatibleFit;
          },
          [](const Abstract& entry)
              -> Utility::Result<
                  const Abstract&, Ttx::Concept::Layout::Errors> {
            return entry;
          });
}

auto Language::Model::Pack::fits(const Ttx::Model::Type& target) const -> Bool {
  BAIL_IF(&resolve() != this);
  const Ttx::Concept::Layout& target_layout = target.get_layout();
  return get_layout().get_size() == target_layout.get_size() &&
         fits_at(target_layout, 0);
}

auto Language::Model::Pack::fits_into(const Ttx::Model::Type& target) const
    -> Bool {
  if (fits(target)) {
    return True;
  }

  auto library_target = target.select<Language::Model::Type>();
  return library_target && library_target->accepts(*this);
}

auto Language::Model::Pack::create_empty(
    Memory::Allocator::Arena& domain,
    Core::Option<Ttx::Lexical::Anchor> anchor) -> Pack& {
  return domain.construct<Group>(
      domain, Core::View::Vector<Reference<Pack>>(),
      Core::View::Vector<Core::View::Bytes>(), anchor);
}

auto Language::Model::Pack::create_group(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Reference<Pack>> entries,
    Core::View::Vector<Core::View::Bytes> names,
    Core::Option<Ttx::Lexical::Anchor> anchor) -> Pack& {
  return domain.construct<Group>(domain, entries, names, anchor);
}

auto Language::Model::Pack::create_folded(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Reference<Pack>> entries) -> Pack& {
  return domain.construct<Group>(
      domain, entries, Core::View::Vector<Core::View::Bytes>(),
      Core::Option<Ttx::Lexical::Anchor>(), True);
}

auto Language::Model::Pack::create_completed(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Reference<Pack>> entries,
    Core::View::Vector<Core::View::Bytes> names) -> Pack& {
  return domain.construct<Group>(
      domain, entries, names, Core::Option<Ttx::Lexical::Anchor>(), True);
}

auto Language::Model::Pack::persist_folded(
    Archive::Writer& writer,
    const Pack& value) -> Bool {
  auto constant = value.select<Language::Constant>();
  if (constant) {
    return constant->persist(writer);
  }

  const Layout& layout = value.get_layout();
  BAIL_IF(layout.is_empty() || layout.get_size() > Unsigned_32(-1));

  Bool named = Bool(layout.get_name(0));
  auto record = writer.begin(Archive::Tag::PackGroup);
  writer.write(Unsigned_32(layout.get_size()));
  writer.write(Unsigned_32(named ? layout.get_size() : 0));
  for (Count index = 0; named && index < layout.get_size(); index++) {
    auto name = layout.get_name(index);
    BAIL_IF(!name || !writer.write(*name));
  }

  for (Count index = 0; index < layout.get_size(); index++) {
    Bool selected_named = Bool(layout.get_name(index));
    auto produced = value.get_produced(index);
    auto selected = produced ? produced->producer.select<Language::Constant>()
                             : Core::Option<const Language::Constant&>();
    BAIL_IF(selected_named != named || !selected || !selected->persist(writer));
  }
  return writer.finish(record);
}

static auto resolve_type(const Abstract& context, Core::View::Bytes name)
    -> Core::Option<const Language::Model::Type&> {
  return context.resolve_context(name)
      .resolve()
      .select<Language::Model::Type>();
}

static auto materialize_type(
    const Abstract& context,
    Core::View::Bytes formula,
    Core::View::Vector<Language::Generic::Argument> arguments)
    -> Core::Option<const Language::Model::Type&> {
  auto generic =
      context.resolve_context(formula).resolve().select<Language::Generic>();
  BAIL_IF(!generic);
  return generic->materialize(arguments).visit(
      [](const Language::Model::Type& selected)
          -> Core::Option<const Language::Model::Type&> { return selected; },
      [](const Language::Generic::Failure&)
          -> Core::Option<const Language::Model::Type&> { return {}; });
}

static auto restore_bytes_type(
    Memory::Allocator::Arena& arena,
    const Abstract& context,
    Count extent) -> Core::Option<const Language::Model::Type&> {
  auto element = resolve_type(context, "Unsigned_8"_view);
  auto fixed = context.resolve_context("Fixed"_view)
                   .resolve()
                   .select<Language::Generic>();
  BAIL_IF(!element || !fixed || extent == 0);

  Core::Static::Vector<Language::Generic::Argument, 2> arguments = {{
    Language::Generic::Argument(*element),
    Language::Generic::Argument(Unsigned_64(extent)),
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

auto Language::Model::Pack::restore_folded(
    Archive::Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& lexical_context) -> Core::Option<Pack&> {
  auto record = reader.read_record();
  BAIL_IF(!record || record->is_optional());

  Archive::Reader contents(record->get_payload());
  Archive::Tag tag = Archive::Tag(record->get_tag());
  switch (tag) {
  case Archive::Tag::PackGroup: {
    auto count = contents.read_unsigned_32();
    auto name_count = contents.read_unsigned_32();
    BAIL_IF(
        !count || *count == 0 || !name_count ||
        (*name_count != 0 && *name_count != *count));

    Memory::Managed::Vector<Core::View::Bytes> names(arena);
    for (Count index = 0; index < *name_count; index++) {
      auto name = contents.read_bytes();
      BAIL_IF(!name);
      names.insert(arena.proxy(*name));
    }

    Memory::Managed::Vector<Reference<Pack>> entries(arena);
    for (Count index = 0; index < *count; index++) {
      auto entry = restore_folded(contents, arena, lexical_context);
      BAIL_IF(!entry);
      entries.insert(*entry);
    }
    BAIL_IF(!contents.is_complete());
    return create_completed(arena, entries.get_view(), names.get_view());
  }
  case Archive::Tag::ConstantFalse:
  case Archive::Tag::ConstantTrue: {
    auto type_name = contents.read_bytes();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto flag = type ? type->select<Language::Model::Types::Flag>()
                     : Core::Option<const Language::Model::Types::Flag&>();
    BAIL_IF(!flag || !contents.is_complete());
    return tag == Archive::Tag::ConstantTrue
               ? static_cast<Pack&>(
                     Language::Constants::True::create_synthetic(arena, *flag))
               : static_cast<Pack&>(
                     Language::Constants::False::create_synthetic(
                         arena, *flag));
  }
  case Archive::Tag::ConstantUnsigned: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_unsigned_64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected =
        type ? type->select<Language::Model::Types::Unsigned>()
             : Core::Option<const Language::Model::Types::Unsigned&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Unsigned::create_synthetic(
        arena, *selected, *value);
  }
  case Archive::Tag::ConstantSigned: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_signed_64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type
                        ? type->select<Language::Model::Types::Signed>()
                        : Core::Option<const Language::Model::Types::Signed&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Signed::create_synthetic(
        arena, *selected, *value);
  }
  case Archive::Tag::ConstantReal: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_real_64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type ? type->select<Language::Model::Types::Real>()
                         : Core::Option<const Language::Model::Types::Real&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Real::create_synthetic(
        arena, *selected, *value);
  }
  case Archive::Tag::ConstantBytes: {
    auto ignored_type_name = contents.read_bytes();
    auto value = contents.read_bytes();
    BAIL_IF(
        !ignored_type_name || !value || value->is_empty() ||
        !contents.is_complete());
    auto type = restore_bytes_type(arena, lexical_context, value->get_size());
    BAIL_IF(!type);
    return Language::Constants::Bytes::create_synthetic(
        arena, *type, arena.proxy(*value));
  }
  case Archive::Tag::ConstantObject: {
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
  case Archive::Tag::ConstantEnumeration: {
    auto type_name = contents.read_bytes();
    auto value = contents.read_unsigned_64();
    auto type = type_name ? resolve_type(lexical_context, *type_name)
                          : Core::Option<const Language::Model::Type&>();
    auto selected = type ? type->select<Language::Types::Enumeration>()
                         : Core::Option<const Language::Types::Enumeration&>();
    BAIL_IF(!value || !selected || !contents.is_complete());
    return Language::Constants::Enumeration::create_synthetic(
        arena, *selected, *value);
  }
  case Archive::Tag::ConstantOption: {
    auto ignored_type_name = contents.read_bytes();
    auto element_name = contents.read_bytes();
    auto present = contents.read_unsigned_8();
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

    auto payload = restore_folded(contents, arena, lexical_context);
    BAIL_IF(!payload || !contents.is_complete());
    auto restored =
        Language::Constants::Option::create_present(arena, *selected, *payload);
    return restored ? Core::Option<Pack&>(*restored) : Core::Option<Pack&>();
  }
  case Archive::Tag::ConstantResult: {
    auto ignored_type_name = contents.read_bytes();
    auto value_name = contents.read_bytes();
    auto error_name = contents.read_bytes();
    auto kind = contents.read_unsigned_8();
    auto value = value_name ? resolve_type(lexical_context, *value_name)
                            : Core::Option<const Language::Model::Type&>();
    auto error = error_name ? resolve_type(lexical_context, *error_name)
                            : Core::Option<const Language::Model::Type&>();
    BAIL_IF(
        !ignored_type_name || !value || !error || !kind ||
        *kind > Unsigned_8(Language::Types::Result::Kind::Error));
    Core::Static::Vector<Language::Generic::Argument, 2> arguments = {{
      Language::Generic::Argument(*value),
      Language::Generic::Argument(*error),
    }};
    auto type =
        materialize_type(lexical_context, "Result"_view, arguments.get_view());
    auto selected = type ? type->select<Language::Types::Result>()
                         : Core::Option<const Language::Types::Result&>();
    auto payload = restore_folded(contents, arena, lexical_context);
    BAIL_IF(!selected || !payload || !contents.is_complete());
    auto restored = *kind == Unsigned_8(Language::Types::Result::Kind::Value)
                        ? Language::Constants::Result::create_value(
                              arena, *selected, *payload)
                        : Language::Constants::Result::create_error(
                              arena, *selected, *payload);
    return restored ? Core::Option<Pack&>(*restored) : Core::Option<Pack&>();
  }
  case Archive::Tag::ConstantRange: {
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
