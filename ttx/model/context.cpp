// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/context.hpp"

#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx;

Model::Context::SnapshotLayout::SnapshotLayout(
    Memory::Allocator::Arena& arena,
    const Concept::Layout& source)
    : abstracts(arena), names(arena) {
  abstracts.reset(source.get_size());
  names.reset(source.get_size());

  for (Count index = 0; index < source.get_size(); index++) {
    const Concept::Abstract& abstract = source.get_abstract(index).visit(
        []() -> const Concept::Abstract& {
          return Concept::Unknown::get_unknown();
        },
        [](const Concept::Abstract& selected) -> const Concept::Abstract& {
          return selected;
        });
    abstracts.insert(abstract);

    auto name = source.get_name(index);
    if (name) {
      names.insert(arena.proxy(*name));
    } else {
      names.insert({});
    }
  }
}

constexpr auto Model::Context::SnapshotLayout::get_abstract(Count index) const
    -> Core::Option<const Concept::Abstract&> {
  BAIL_IF(index >= abstracts.get_size());
  return abstracts.at(index).get();
}

constexpr auto Model::Context::SnapshotLayout::get_name(Count index) const
    -> Core::Option<Core::View::Bytes> {
  BAIL_IF(index >= names.get_size());
  return names.at(index);
}

auto Model::Context::SnapshotLayout::fits_entry(
    const Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  return source && destination && &source->resolve() == &destination->resolve();
}

auto Model::Context::SnapshotLayout::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Model::Context::SnapshotLayout::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const
    -> Utility::Result<const Concept::Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }
  return abstracts.at(target_index).get();
}

auto Model::Context::pack(const Concept::Layout& source)
    -> const Concept::Pack& {
  auto& layout = arena.construct<SnapshotLayout>(arena, source);
  return arena.construct<SnapshotPack>(layout);
}
