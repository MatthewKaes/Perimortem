// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/aggregate.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Constants::Aggregate::create(
    Memory::Allocator::Arena& arena,
    Core::View::Vector<Language::Model::Pack*> values,
    Core::View::Vector<Core::View::Bytes> names) -> Core::Option<Aggregate&> {
  BAIL_IF(!names.is_empty() && names.get_size() != values.get_size());
  for (Language::Model::Pack* value : values) {
    auto identity = value->get_identity();
    BAIL_IF(
        !value->is_complete() || value->get_layout().get_size() != 1 ||
        !identity || !identity->is<Ttx::Concept::Constant>());
  }
  return arena.construct_from<Aggregate>(
      [&]() -> Aggregate { return Aggregate(arena, values, names); });
}

Language::Constants::Aggregate::Aggregate(
    Memory::Allocator::Arena& arena,
    Core::View::Vector<Language::Model::Pack*> source,
    Core::View::Vector<Core::View::Bytes> source_names)
    : values(arena), names(arena), name(arena), layout(*this) {
  values.reset(source.get_size());
  for (Language::Model::Pack* value : source) {
    values.insert(value);
  }
  names.reset(source_names.get_size());
  for (Core::View::Bytes value : source_names) {
    names.insert(arena.proxy(value));
  }

  Serialization::Stream::Textual<Memory::Managed::Bytes> output(name);
  output << "["_view;
  for (Count index = 0; index < values.get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }
    if (!names.is_empty()) {
      output << "."_view << names.at(index) << " = "_view;
    }
    output << values.at(index)->get_identity()->get_name();
  }
  output << "]"_view;
}

auto Language::Constants::Aggregate::get_name() const -> Core::View::Bytes {
  return name.get_view();
}

auto Language::Constants::Aggregate::get_type() const -> const Abstract& {
  return values.get_size() == 1
             ? values.at(0)->get_type()
             : static_cast<const Abstract&>(None::get_none());
}

auto Language::Constants::Aggregate::get_result() const -> const Abstract& {
  return *this;
}

auto Language::Constants::Aggregate::get_identity() const
    -> Core::Option<const Abstract&> {
  return *this;
}

auto Language::Constants::Aggregate::get_layout() const
    -> const Ttx::Concept::Layout& {
  return layout;
}

auto Language::Constants::Aggregate::get_value_type(Count index) const
    -> const Abstract& {
  return index < values.get_size()
             ? values.at(index)->get_value_type(0)
             : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Language::Constants::Aggregate::fits(
    const Ttx::Concept::Layout& target) const -> Bool {
  return Model::Pack::fits(target);
}

auto Language::Constants::Aggregate::fits(const Ttx::Model::Type& target) const
    -> Bool {
  return Model::Pack::fits(target);
}

auto Language::Constants::Aggregate::link(
    Ttx::Lexical::Cursor&,
    const Abstract&,
    Core::Option<const Abstract&>) -> Bool {
  return True;
}

auto Language::Constants::Aggregate::finalize(Ttx::Lexical::Cursor&) -> void {}

auto Language::Constants::Aggregate::Layout::get_size() const -> Count {
  return aggregate.values.get_size();
}

auto Language::Constants::Aggregate::Layout::get_abstract(Count index) const
    -> Core::Option<const Abstract&> {
  BAIL_IF(index >= get_size());
  return aggregate.values.at(index)->get_identity();
}

auto Language::Constants::Aggregate::Layout::get_name(Count index) const
    -> Core::Option<Core::View::Bytes> {
  BAIL_IF(aggregate.names.is_empty() || index >= aggregate.names.get_size());
  return aggregate.names.at(index);
}

auto Language::Constants::Aggregate::Layout::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size());
  return aggregate.values.at(source_index)->fits_entry(target, 0, target_index);
}

auto Language::Constants::Aggregate::Layout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Language::Constants::Aggregate::Layout::get_fitted_at(
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
  auto selected = get_abstract(target_index);
  return selected ? Utility::Result<const Abstract&, Errors>(*selected)
                  : Utility::Result<const Abstract&, Errors>(
                        Errors::IncompatibleFit);
}
