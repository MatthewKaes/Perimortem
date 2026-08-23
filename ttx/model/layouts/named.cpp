// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx;
using namespace Ttx::Model;

static auto get_slot_name(const Concept::Layout& layout, Count index)
    -> Option<View::Bytes> {
  auto explicit_name = layout.get_name(index);
  if (explicit_name) {
    return explicit_name;
  }

  return layout.get_abstract(index).visit(
      []() -> Option<View::Bytes> { return {}; },
      [](const Concept::Abstract& selected) -> Option<View::Bytes> {
        View::Bytes name = selected.get_name();
        return name.is_empty() ? Option<View::Bytes>()
                               : Option<View::Bytes>(name);
      });
}

constexpr auto Layouts::Named::get_name(Count index) const
    -> Option<View::Bytes> {
  BAIL_IF(index >= get_size());

  View::Bytes selected = names.visit(
      [&]() {
        return get_source().get_abstract(index).visit(
            []() { return View::Bytes(); },
            [](const Concept::Abstract& abstract) {
              return abstract.get_name();
            });
      },
      [&](View::Vector<View::Bytes> explicit_names) {
        return index < explicit_names.get_size()
                   ? explicit_names.get_data()[index]
                   : View::Bytes();
      });
  BAIL_IF(selected.is_empty());
  return selected;
}

auto Layouts::Named::has_unique_names() const -> Bool {
  for (Count index = 0; index < get_size(); index++) {
    auto name = get_name(index);
    BAIL_IF(!name);

    for (Count other = index + 1; other < get_size(); other++) {
      auto candidate = get_name(other);
      BAIL_IF(!candidate || *name == *candidate);
    }
  }

  return names.visit(
      []() { return True; },
      [&](View::Vector<View::Bytes> explicit_names) {
        return explicit_names.get_size() == get_size() ? True : False;
      });
}

auto Layouts::Named::fits_entry(
    const Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  auto source_name = get_name(source_index);
  auto target_name = get_slot_name(target, target_index);
  return source_name && target_name && *source_name == *target_name &&
         get_source().fits_entry(target, source_index, target_index);
}

auto Layouts::Named::fits_at(const Concept::Layout& target, Count target_offset)
    const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset) || !has_unique_names());

  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    BAIL_IF(!source_name);

    Count selected = 0;
    Count matches = 0;
    for (Count target_index = 0; target_index < get_size(); target_index++) {
      auto target_name = get_slot_name(target, target_offset + target_index);
      if (target_name && *source_name == *target_name) {
        selected = target_index;
        matches++;
      }
    }
    BAIL_IF(
        matches != 1 || !get_source().fits_entry(
                            target, source_index, target_offset + selected));
  }

  return True;
}

auto Layouts::Named::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Concept::Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  auto target_name = get_slot_name(target, target_offset + target_index);
  if (!target_name) {
    return Errors::IncompatibleFit;
  }

  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    if (source_name && *source_name == *target_name) {
      return get_source()
          .get_abstract(source_index)
          .visit(
              []() -> Result<const Concept::Abstract&, Errors> {
                return Errors::IncompatibleFit;
              },
              [](const Concept::Abstract& selected)
                  -> Result<const Concept::Abstract&, Errors> {
                return selected;
              });
    }
  }

  return Errors::IncompatibleFit;
}
