// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/type_reference.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

static auto select_terminal(const Abstract& binding, View::Bytes name)
    -> const Abstract& {
  const Abstract& resolved = resolve_alias(binding);
  if (resolved.is<Invalid>() || resolved.is<Ttx::Model::Type>()) {
    return resolved;
  }

  // A Package Source Alias keeps its Monograph visible to ordinary queries.
  // A Type route may still select the matching root Type published by that
  // Monograph, which preserves both observations without copying the Type into
  // Package.
  const Abstract& nested = resolve_alias(resolved.resolve_context(name));
  return nested.is<Ttx::Model::Type>() ? nested : resolved;
}

static auto resolve_route(
    View::Bytes route,
    const Abstract& context,
    Option<Cursor&> cursor,
    Anchor anchor) -> Option<const Ttx::Model::Type&> {
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
    const Abstract* candidate = &selected->resolve_context(name).resolve();
    if (candidate->is<Invalid>()) {
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
  const Abstract& resolved = select_terminal(*selected, terminal_name);
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
    cursor->get_associations().create(anchor, *type);
  }
  return *type;
}

auto Language::TypeReference::resolve(Cursor& cursor, const Abstract& context)
    const -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, context, cursor, anchor);
}

auto Language::TypeReference::resolve_restored(const Abstract& context) const
    -> Option<const Ttx::Model::Type&> {
  return resolve_route(route, context, {}, anchor);
}
