// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

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

   private:
    auto select(Count index) const -> Core::Option<Selection>;

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

  TTX_CONTRACT(
      Group,
      Language::Model::Pack,
      0x878367a4aaf04e9f,
      0xbb415be4c70a29db);

  TTX_NAME("Pack"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Abstract& lexical_context,
      Core::Option<const Type&> access_scope) -> Bool override {
    Bool failed = False;
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      failed |= !entry.get().link(source, lexical_context, access_scope);
    }
    BAIL_IF(failed);

    if (!names.is_empty()) {
      for (Reference<Language::Model::Pack> entry : entries.get_view()) {
        if (entry.get().get_layout().get_size() != 1) {
          source.report(
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

  auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }

  auto resolve() const -> const Abstract& override {
    return linked ? static_cast<const Language::Model::Pack&>(*this)
                  : static_cast<const Abstract&>(Invalid::get_invalid());
  }

  auto finalize() -> void override {
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      entry.get().finalize();
    }
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

static auto get_target_name(const Layout& target, Count index)
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
  const Layout& layout = get_layout();
  if (layout.get_size() != 1) {
    return Invalid::get_invalid();
  }

  return layout.get_abstract(0).visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Abstract& entry) -> const Abstract& {
        auto pack = entry.select<Language::Model::Pack>();
        if (pack) {
          return pack->get_type();
        }

        auto addressable = entry.select<Addressable>();
        if (addressable) {
          return addressable->get_type();
        }

        auto type = entry.select<Type>();
        return type ? static_cast<const Abstract&>(*type)
                    : static_cast<const Abstract&>(Invalid::get_invalid());
      });
}

static auto select_target_type(const Abstract& target)
    -> Core::Option<const Type&> {
  auto direct = target.select<Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = target.resolve();
  auto addressable = resolved.select<Addressable>();
  const Abstract& selected = addressable ? addressable->get_type() : resolved;
  direct = selected.select<Type>();
  return direct ? direct : selected.resolve().select<Type>();
}

auto Language::Model::Pack::fits_entry(
    const Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  const Layout& source = get_layout();
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

  auto producer =
      source.get_abstract(source_index)
          .visit(
              []() -> Core::Option<const Language::Model::Pack&> { return {}; },
              [](const Abstract& entry)
                  -> Core::Option<const Language::Model::Pack&> {
                return entry.select<Language::Model::Pack>();
              });
  return producer ? producer->fits(*target_type) : False;
}

auto Language::Model::Pack::fits_at(const Layout& target, Count target_offset)
    const -> Bool {
  const Layout& source = get_layout();
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

auto Language::Model::Pack::fits(const Layout& target) const -> Bool {
  return get_layout().get_size() == target.get_size() && fits_at(target, 0);
}

auto Language::Model::Pack::get_fitted_at(
    const Layout& target,
    Count target_offset,
    Count target_index) const
    -> Utility::Result<const Abstract&, Layout::Errors> {
  const Layout& source = get_layout();
  if (target_index >= source.get_size()) {
    return Layout::Errors::IndexOutOfBounds;
  }
  if (target_offset > target.get_size() ||
      source.get_size() > target.get_size() - target_offset) {
    return Layout::Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Layout::Errors::IncompatibleFit;
  }

  for (Count index = 0; index < source.get_size(); index++) {
    if (source.get_name(index)) {
      return source.get_fitted_at(target, target_offset, target_index);
    }
  }
  return source.get_abstract(target_index)
      .visit(
          []() -> Utility::Result<const Abstract&, Layout::Errors> {
            return Layout::Errors::IncompatibleFit;
          },
          [](const Abstract& entry)
              -> Utility::Result<const Abstract&, Layout::Errors> {
            return entry;
          });
}

auto Language::Model::Pack::fits(const Type& target) const -> Bool {
  return fits(target.get_layout());
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
