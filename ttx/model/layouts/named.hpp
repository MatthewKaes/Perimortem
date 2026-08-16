// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Ttx::Model::Layouts {

// Named is a descriptor with one nonempty unique name per source slot. A Pack
// may expose it for authored named flow, while a Callable or Type may expose it
// as a required contract. Names may come from the retained Abstracts or from an
// aligned slot-name view supplied by the owner. In either form fitted queries
// return the original source Abstract and create no renamed semantic identity.
class Named : public Concept::Layout {
 public:
  constexpr Named(
      Perimortem::Core::View::Vector<
          Concept::Reference<const Concept::Abstract>> abstracts = {})
      : values(abstracts) {}

  constexpr Named(
      const Concept::Layout& values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names)
      : values(), source(values), names(names) {}

  constexpr auto get_size() const -> Count override {
    return get_source().get_size();
  }
  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Concept::Abstract&> override {
    return get_source().get_abstract(index);
  }

  constexpr auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;

  auto fits_entry(
      const Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool override;

  auto fits_at(const Concept::Layout& target, Count target_offset) const
      -> Bool override;
  auto get_fitted_at(
      const Concept::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> override;

 private:
  constexpr auto get_source() const -> const Concept::Layout& {
    return source.visit(
        [&]() -> const Concept::Layout& { return values; },
        [](const Concept::Layout& selected) -> const Concept::Layout& {
          return selected;
        });
  }

  auto has_unique_names() const -> Bool;

  Fluid values;
  Perimortem::Core::Option<const Concept::Layout&> source;
  Perimortem::Core::Option<
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>>
      names;
};

}  // namespace Ttx::Model::Layouts
