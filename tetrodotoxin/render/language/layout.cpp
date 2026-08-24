// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/layout.hpp"

#include "tetrodotoxin/render/language/attributes.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Layout::create(
    Allocator::Arena& domain,
    Managed::Vector<Slot> slots,
    Anchor anchor,
    Bool parameters) -> Layout& {
  return domain.construct_from<Layout>(
      [&]() { return Layout(domain, slots, anchor, parameters); });
}

auto Language::Layout::link(Cursor& cursor, const Abstract& context) -> Bool {
  Bool valid = True;
  for (Count index = 0; index < slots.get_size(); index++) {
    Slot& slot = slots[index];
    valid &= Attributes::validate(
        cursor, slot.get_attributes(), Attributes::Placement::StageEntry);
    auto selected = slot.get_type().resolve(cursor, context);
    if (!selected || selected->get_layout().is_empty()) {
      valid = False;
      continue;
    }

    auto retained = slot.get_edge();
    if (retained) {
      auto parameter = retained->select<Ttx::Model::Addressable>();
      Bool stable =
          parameters ? Bool(parameter && &parameter->get_type() == &*selected)
                     : &*retained == &*selected;
      if (!stable) {
        cursor.create_expression_error(
            slot.get_anchor(),
            "Repeated Render Layout linking selected a different semantic edge."_view);
        valid = False;
      }
      continue;
    }

    if (parameters) {
      if (slot.get_name().is_empty()) {
        cursor.create_expression_error(
            slot.get_anchor(),
            "Render Stage parameters require one name for every entry."_view);
        valid = False;
        continue;
      }
      auto& binding = Binding::create_slot(domain, slot.get_name(), *selected);
      valid &= slot.retain_edge(binding);
      cursor.get_associations().create(slot.get_anchor(), binding);
    } else {
      valid &= slot.retain_edge(*selected);
    }
  }

  for (Count index = 0; index < slots.get_size(); index++) {
    if (slots[index].get_name().is_empty()) {
      continue;
    }
    for (Count prior = 0; prior < index; prior++) {
      if (slots[prior].get_name() == slots[index].get_name()) {
        cursor.create_expression_error(
            slots[index].get_anchor(),
            "Render Stage Layout names are unique within one side."_view);
        valid = False;
      }
    }
  }

  return valid && is_linked();
}

auto Language::Layout::link_restored(const Abstract& context) -> Bool {
  for (Count index = 0; index < slots.get_size(); index++) {
    Slot& slot = slots[index];
    BAIL_IF(
        !Attributes::accepts(
            slot.get_attributes(), Attributes::Placement::StageEntry) ||
        slot.get_edge());
    auto selected = slot.get_type().resolve_restored(context);
    BAIL_IF(!selected || selected->get_layout().is_empty());

    if (parameters) {
      BAIL_IF(slot.get_name().is_empty());
      auto& binding = Binding::create_slot(domain, slot.get_name(), *selected);
      BAIL_IF(!slot.retain_edge(binding));
    } else {
      BAIL_IF(!slot.retain_edge(*selected));
    }
  }

  for (Count index = 0; index < slots.get_size(); index++) {
    if (slots[index].get_name().is_empty()) {
      continue;
    }
    for (Count prior = 0; prior < index; prior++) {
      BAIL_IF(slots[prior].get_name() == slots[index].get_name());
    }
  }
  return is_linked();
}

auto Language::Layout::resolve_named(View::Bytes name) const
    -> const Abstract& {
  for (Count index = 0; index < slots.get_size(); index++) {
    auto edge = slots.at(index).get_edge();
    if (slots.at(index).get_name() == name && edge) {
      return *edge;
    }
  }
  return Invalid::get_invalid();
}

auto Language::Layout::is_linked() const -> Bool {
  for (Count index = 0; index < slots.get_size(); index++) {
    if (!slots.at(index).get_edge()) {
      return False;
    }
  }
  return True;
}

auto Language::Layout::get_size() const -> Count {
  return slots.get_size();
}

auto Language::Layout::get_abstract(Count index) const
    -> Option<const Abstract&> {
  BAIL_IF(index >= slots.get_size());
  return slots.at(index).get_edge();
}

auto Language::Layout::get_name(Count index) const -> Option<View::Bytes> {
  BAIL_IF(index >= slots.get_size() || slots.at(index).get_name().is_empty());
  return slots.at(index).get_name();
}

static auto represented_type(const Abstract& value) -> const Abstract& {
  auto addressable = value.select<Ttx::Model::Addressable>();
  return addressable ? static_cast<const Abstract&>(addressable->get_type())
                     : value.resolve();
}

auto Language::Layout::find_target(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_offset) const -> Option<Count> {
  BAIL_IF(source_index >= get_size());
  auto source_name = get_name(source_index);
  if (!source_name) {
    Count selected = target_offset + source_index;
    return selected < target.get_size() ? Option<Count>(selected)
                                        : Option<Count>();
  }

  Option<Count> selected;
  for (Count index = target_offset; index < target.get_size(); index++) {
    auto target_name = target.get_name(index);
    if (target_name && *target_name == *source_name) {
      BAIL_IF(selected);
      selected = index;
    }
  }
  return selected;
}

auto Language::Layout::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  BAIL_IF(!source || !destination);
  return &represented_type(*source) == &represented_type(*destination);
}

auto Language::Layout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    auto target_index = find_target(target, index, target_offset);
    BAIL_IF(!target_index || !fits_entry(target, index, *target_index));
  }
  return True;
}

auto Language::Layout::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  auto selected = find_target(target, target_index, target_offset);
  if (!selected || !fits_entry(target, target_index, *selected)) {
    return Errors::IncompatibleFit;
  }
  return *get_abstract(target_index);
}
