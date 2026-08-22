// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/lexical/associations.hpp"

using namespace Perimortem::Core;
using namespace Ttx;

static constexpr auto contains(Lexical::Token token, Count offset) -> Bool {
  return token && offset >= token.get_offset() &&
         offset < Count(token.get_offset()) + Count(token.get_size());
}

static constexpr auto contains(Lexical::Span span, Count offset) -> Bool {
  return span && offset >= span.get_offset() &&
         offset < Count(span.get_offset()) + span.get_size();
}

auto Lexical::Associations::create(
    Anchor anchor,
    const Concept::Abstract& semantic) -> void {
  if (!anchor.get_span()) {
    return;
  }

  associations.insert({
    anchor,
    semantic,
  });
}

auto Lexical::Associations::find_at(Count offset) const
    -> Option<const Concept::Abstract&> {
  Option<const Associations::Entry&> selected;
  Bool selected_focus = False;
  Count selected_extent = Count(-1);

  auto source_associations = associations.get_view();
  for (Count i = 0; i < source_associations.get_size(); i++) {
    const Associations::Entry& association = source_associations.get_data()[i];
    Lexical::Anchor anchor = association.get_anchor();
    Lexical::Token focus = anchor.get_token();
    Lexical::Span span = anchor.get_span();
    Bool contains_focus = contains(focus, offset);
    Bool contains_span = contains(span, offset);
    if (!contains_focus && !contains_span) {
      continue;
    }

    Count extent = contains_focus ? focus.get_size() : span.get_size();
    if (!selected || (contains_focus && !selected_focus) ||
        (contains_focus == selected_focus && extent < selected_extent)) {
      selected = association;
      selected_focus = contains_focus;
      selected_extent = extent;
    }
  }

  if (!selected) {
    return {};
  }

  return selected->get_semantic();
}

auto Lexical::Associations::find(const Concept::Abstract& semantic) const
    -> Option<Lexical::Anchor> {
  for (const Entry& association : associations.get_view()) {
    if (&association.get_semantic() == &semantic) {
      return association.get_anchor();
    }
  }

  return {};
}
