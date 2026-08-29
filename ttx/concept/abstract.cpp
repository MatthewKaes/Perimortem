// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Ttx::Concept;

class EmptyConceptLayout : public Layout {
 public:
  constexpr auto get_size() const -> Count override { return 0; }
  constexpr auto get_abstract(Count) const
      -> Perimortem::Core::Option<const Abstract&> override {
    return {};
  }
  constexpr auto fits_entry(const Layout&, Count, Count) const
      -> Bool override {
    return False;
  }
  constexpr auto fits_at(const Layout& target, Count target_offset) const
      -> Bool override {
    return target_offset <= target.get_size();
  }
  constexpr auto get_fitted_at(const Layout&, Count, Count) const
      -> Perimortem::Utility::Result<const Abstract&, Errors> override {
    return Errors::IndexOutOfBounds;
  }
};

auto Abstract::get_type() const -> const Abstract& {
  return None::get_none();
}

auto Abstract::resolve_concept(Perimortem::Core::View::Bytes) const
    -> const Abstract& {
  return None::get_none();
}

auto Abstract::get_concepts(Context& context) const -> const Pack& {
  static constexpr EmptyConceptLayout empty;
  return context.pack(empty);
}

auto Abstract::satisfies(const Abstract&) const -> Bool {
  return False;
}
