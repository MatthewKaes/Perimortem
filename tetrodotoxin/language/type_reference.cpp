// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/type_reference.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto is_missing(const Abstract& abstract) -> Bool {
  return abstract.is<Unknown>() || abstract.is<None>();
}

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Language::Import>(
      [](const Language::Import& import) -> const Abstract& {
        return import.resolve();
      },
      [](const Abstract& candidate) -> const Abstract& {
        return candidate.visit<Ttx::Model::Alias>(
            [](const Ttx::Model::Alias& alias) -> const Abstract& {
              return alias.resolve();
            },
            [](const Abstract& direct) -> const Abstract& { return direct; });
      });
}

static auto segment_anchor(
    Anchor route_anchor,
    View::Bytes route,
    View::Bytes name) -> Anchor {
  Token first = route_anchor.get_token();
  if (!first || name.is_empty()) {
    return Anchor::create(Span());
  }

  Count offset = Count(name.get_data() - route.get_data());
  Token token(
      U16(Count(first.get_offset()) + offset), first.get_line(),
      U16(Count(first.get_column()) + offset), U8(name.get_size()),
      first.get_code());
  return Anchor::create(token, Span(token));
}

static auto resolve_route(
    View::Bytes route,
    const Abstract& context,
    Option<Cursor&> cursor,
    Anchor anchor,
    Option<const Abstract&> supplied_root = {})
    -> Option<const Ttx::Model::Type&> {
  const Abstract* selected = &context;
  Count start = 0;
  Count segment = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes name = route.slice(start, index - start);
    const Abstract* queried = nullptr;
    if (segment == 0 && supplied_root) {
      queried = &*supplied_root;
    } else if (segment == 0) {
      queried = &context.visit<Language::Monograph>(
          [&](const Language::Monograph& monograph) -> const Abstract& {
            return monograph.resolve_lexical_context(name);
          },
          [&](const Abstract&) -> const Abstract& {
            return selected->resolve_concept(name);
          });
    } else {
      queried = &selected->resolve_concept(name);
    }

    const Abstract& represented = resolve_alias(*queried);
    const Abstract* candidate =
        queried->is<Ttx::Model::Type>() && !queried->is<Language::Import>()
            ? queried
            : &represented;
    if (is_missing(*candidate)) {
      if (cursor) {
        auto report = cursor->create_report(anchor);
        report << "Type route `"_view << route
               << "` could not resolve segment "_view << U64(segment)
               << "."_view;
        report.get_hint()
            << "Publish that Type in the selected semantic context."_view;
      }
      return {};
    }
    if (cursor && !terminal) {
      cursor->get_associations().create(
          segment_anchor(anchor, route, name), *queried);
    }
    selected = candidate;
    segment++;

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  View::Bytes terminal_name;
  Count terminal_start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (separator) {
      index++;
      terminal_start = index + 1;
    } else if (terminal) {
      terminal_name = route.slice(terminal_start, index - terminal_start);
    }
  }
  const Abstract& resolved = resolve_alias(*selected);
  auto type = resolved.select<Ttx::Model::Type>();
  if (!type) {
    if (cursor) {
      auto report = cursor->create_report(anchor);
      report << "Route `"_view << route << "` does not select one Type."_view;
      report.get_hint()
          << "Select one value Type rather than a contextual declaration."_view;
    }
    return {};
  }

  if (cursor) {
    cursor->get_associations().create(
        segment_anchor(anchor, route, terminal_name), *type);
  }
  return *type;
}

auto Language::TypeReference::resolve(Cursor& cursor, const Abstract& context)
    const -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, context, cursor, anchor);
}

auto Language::TypeReference::resolve_selected(
    Cursor& cursor,
    const Abstract& selected_root) const -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, selected_root, cursor, anchor, selected_root);
}

auto Language::TypeReference::resolve_restored(const Abstract& context) const
    -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, context, {}, anchor);
}

auto Language::TypeReference::resolve_restored_selected(
    const Abstract& selected_root) const -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, selected_root, {}, anchor, selected_root);
}

auto Language::TypeReference::get_root() const -> View::Bytes {
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    if (route[index] == ':' && route[index + 1] == ':') {
      return route.slice(0, index);
    }
  }

  return route;
}
