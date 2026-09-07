// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/type_reference.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem::Core;

static auto query_route(View::Bytes route, ttx_abstract host, bool supplied)
    -> ttx_abstract {
  ttx_abstract selected = Ttx::resolve(host);
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); ++index) {
    const bool end = index == route.get_size();
    const bool separator = !end && index + 1 < route.get_size() &&
                           route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }
    auto name = route.slice(start, index - start);
    while (!name.is_empty() && (name[0] == ' ' || name[0] == '\t')) {
      name = name.slice(1, name.get_size() - 1);
    }
    while (!name.is_empty() && (name[name.get_size() - 1] == ' ' ||
                                name[name.get_size() - 1] == '\t')) {
      name = name.slice(0, name.get_size() - 1);
    }
    if (!(start == 0 && supplied)) {
      selected = Ttx::resolve(
          Ttx::resolve_concept(selected, {name.get_data(), name.get_size()}));
    }
    if (separator) {
      ++index;
      start = index + 1;
    }
  }
  if (ttx_abstract_same(selected, ttx_unknown()) ||
      ttx_abstract_same(selected, ttx_none())) {
    return selected;
  }
  const auto domain = Ttx::resolve_domain(selected);
  // A type spelling asks for the Domain itself. A value whose Domain happens
  // to match cannot be promoted into that identity by inspecting a native
  // class.
  if (domain.state == Ttx::Observation::Unknown) {
    return ttx_unknown();
  }
  return domain.state == Ttx::Observation::Resolved &&
                 ttx_abstract_same(domain.domain, selected)
             ? selected
             : ttx_none();
}

auto TypeReference::resolve(Ttx::Lexical::Cursor& cursor, ttx_abstract context)
    const -> ttx_abstract {
  const auto selected = query_route(route, context, false);
  cursor.get_associations().create(anchor, selected);
  return selected;
}

auto TypeReference::resolve_selected(
    Ttx::Lexical::Cursor& cursor,
    ttx_abstract selected_root) const -> ttx_abstract {
  const auto selected = query_route(route, selected_root, true);
  cursor.get_associations().create(anchor, selected);
  return selected;
}

auto TypeReference::resolve_restored(ttx_abstract context) const
    -> ttx_abstract {
  return query_route(route, context, false);
}

auto TypeReference::resolve_restored_selected(ttx_abstract selected_root) const
    -> ttx_abstract {
  return query_route(route, selected_root, true);
}

auto TypeReference::get_root() const -> View::Bytes {
  for (Count index = 0; index + 1 < route.get_size(); ++index) {
    if (route[index] == ':' && route[index + 1] == ':') {
      return route.slice(0, index);
    }
  }
  return route;
}
