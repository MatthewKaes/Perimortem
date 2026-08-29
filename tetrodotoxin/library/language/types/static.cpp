// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/static.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Types::Static::can_bind(const Abstract& binding) const -> Bool {
  Core::View::Bytes name = binding.get_name();
  BAIL_IF(name.is_empty());

  auto selected = bindings.find(name);
  return !selected || &selected->value.semantic.get() == &binding;
}

auto Types::Static::bind(Abstract& binding, Bool published) -> Bool {
  BAIL_IF(!can_bind(binding));

  auto selected = bindings.find(binding.get_name());
  if (selected) {
    selected->value.published |= published;
    return True;
  }

  return bindings.insert(binding.get_name(), Binding(binding, published)) !=
         nullptr;
}

auto Types::Static::is_published(const Abstract& binding) const -> Bool {
  auto selected = bindings.find(binding.get_name());
  return selected && &selected->value.semantic.get() == &binding &&
         selected->value.published;
}

auto Types::Static::resolve_published(Core::View::Bytes name) const
    -> const Abstract& {
  auto selected = bindings.find(name);
  if (selected && selected->value.published) {
    return selected->value.semantic.get();
  }
  return selected || completed ? static_cast<const Abstract&>(None::get_none())
                               : Unknown::get_unknown();
}

auto Types::Static::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  auto selected = bindings.find(name);
  if (selected) {
    return selected->value.semantic.get();
  }
  return completed ? static_cast<const Abstract&>(None::get_none())
                   : Unknown::get_unknown();
}

auto Types::Static::get_concepts(Context& context) const -> const Pack& {
  Surface surface(*this);
  return context.pack(surface);
}

auto Types::Static::Surface::select(Count index) const -> const Binding* {
  for (Count entry = 0; entry < owner.bindings.get_size(); entry++) {
    const auto* selected = owner.bindings.get_entry(entry);
    if (selected != nullptr && selected->value.published) {
      if (index == 0) {
        return &selected->value;
      }
      index--;
    }
  }
  return nullptr;
}

auto Types::Static::Surface::get_size() const -> Count {
  Count size = 0;
  for (Count entry = 0; entry < owner.bindings.get_size(); entry++) {
    const auto* selected = owner.bindings.get_entry(entry);
    if (selected != nullptr && selected->value.published) {
      size++;
    }
  }
  return size;
}

auto Types::Static::Surface::get_abstract(Count index) const
    -> Core::Option<const Abstract&> {
  const Binding* selected = select(index);
  return selected ? Core::Option<const Abstract&>(selected->semantic.get())
                  : Core::Option<const Abstract&>();
}

auto Types::Static::Surface::get_name(Count index) const
    -> Core::Option<Core::View::Bytes> {
  const Binding* selected = select(index);
  return selected ? Core::Option<Core::View::Bytes>(
                        selected->semantic.get().get_name())
                  : Core::Option<Core::View::Bytes>();
}

auto Types::Static::Surface::fits_entry(
    const Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  return source && destination && &source->resolve() == &destination->resolve();
}

auto Types::Static::Surface::fits_at(const Layout& target, Count target_offset)
    const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Types::Static::Surface::get_fitted_at(
    const Layout& target,
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
  const Binding* selected = select(target_index);
  return selected ? Utility::Result<const Abstract&, Errors>(
                        selected->semantic.get())
                  : Utility::Result<const Abstract&, Errors>(
                        Errors::IncompatibleFit);
}
