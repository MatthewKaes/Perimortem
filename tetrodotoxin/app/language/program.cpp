// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/program.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::App;

static auto resolve_route(const Abstract& context, View::Bytes route)
    -> const Abstract& {
  const Abstract* selected = &context;
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes segment = route.slice(start, index - start);
    if (segment.is_empty()) {
      return Unknown::get_unknown();
    }

    const Abstract& queried =
        selected->visit<Tetrodotoxin::Language::Monograph>(
            [&](const Tetrodotoxin::Language::Monograph& monograph)
                -> const Abstract& {
              return start == 0 ? monograph.resolve_lexical_context(segment)
                                : monograph.resolve_concept(segment);
            },
            [&](const Abstract& selected_context) -> const Abstract& {
              return selected_context.resolve_concept(segment);
            });
    const Abstract& candidate = queried.resolve();
    if (candidate.is<Unknown>() || candidate.is<None>()) {
      return candidate;
    }
    selected = &candidate;

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  return *selected;
}

static auto select_entry(
    const Abstract& context,
    View::Bytes route,
    View::Bytes callable_name) -> Option<const Ttx::Model::Callable&> {
  const Abstract& receiver = resolve_route(context, route);
  BAIL_IF(receiver.is<Unknown>() || receiver.is<None>());
  const Abstract& selected = receiver.resolve_concept("static"_view)
                                 .resolve_concept(callable_name)
                                 .resolve();
  auto callable = selected.select<Ttx::Model::Callable>();
  BAIL_IF(
      !callable || !callable->get_parameters().is_empty() ||
      !callable->get_results().is_empty());
  return *callable;
}

auto Language::Program::create_authored(
    Perimortem::Memory::Allocator::Arena& arena,
    const Documentation& documentation,
    View::Bytes route,
    View::Bytes callable_name,
    Anchor anchor,
    Anchor selection_anchor) -> Program& {
  return arena.construct_from<Program>([&]() {
    return Program(
        documentation, route, callable_name, anchor, selection_anchor);
  });
}

auto Language::Program::create_synthetic(
    Perimortem::Memory::Allocator::Arena& arena,
    const Documentation& documentation,
    View::Bytes route,
    View::Bytes callable_name) -> Program& {
  return arena.construct_from<Program>([&]() {
    return Program(
        documentation, arena.proxy(route), arena.proxy(callable_name),
        Anchor::create({}), Anchor::create({}));
  });
}

auto Language::Program::link(Cursor& cursor, Abstract& context) -> Bool {
  const Abstract& receiver = resolve_route(context, route);
  if (receiver.is<Unknown>() || receiver.is<None>()) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry route `"_view << route
           << "` does not resolve in this Package."_view;
    report.get_hint()
        << "Select one exact Package member or nested public context."_view;
    return False;
  }

  const Abstract& selected = receiver.resolve_concept("static"_view)
                                 .resolve_concept(callable_name)
                                 .resolve();
  auto callable = selected.select<Ttx::Model::Callable>();
  if (!callable) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry `"_view << route << " -> "_view << callable_name
           << "` does not select a Static Callable."_view;
    report.get_hint()
        << "Publish one Callable with empty parameters and results."_view;
    return False;
  }

  if (!callable->get_parameters().is_empty() ||
      !callable->get_results().is_empty()) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry Callable `"_view << callable_name
           << "` must have empty parameter and result Layouts."_view;
    report.get_hint() << "Use a Static `[] -> []` Callable."_view;
    return False;
  }

  entry = &*callable;
  cursor.get_associations().create(selection_anchor, *callable);
  return True;
}

auto Language::Program::link_restored(Abstract& context) -> Bool {
  auto callable = select_entry(context, route, callable_name);
  if (!callable) {
    Diagnostics::Log::error(
        "Restored App Program entry does not resolve to `[] -> []`."_view);
    return False;
  }

  entry = &*callable;
  return True;
}

auto Language::Program::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
