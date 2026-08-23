// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/program.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::App;

static auto require_text(
    Cursor& cursor,
    Code::Type code,
    View::Bytes text,
    View::Bytes message) -> Token {
  if (!cursor.matches(code) || cursor.get_text() != text) {
    cursor.create_token_error(cursor.current(), message);
    return {};
  }

  return cursor.consume();
}

static auto resolve_route(const Abstract& context, View::Bytes route)
    -> const Abstract& {
  Reference<const Abstract> selected(context);
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
      return Invalid::get_invalid();
    }

    const Abstract& candidate =
        selected.get().resolve_context(segment).resolve();
    if (candidate.is<Invalid>()) {
      return candidate;
    }
    selected = Reference<const Abstract>(candidate);

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  return selected.get();
}

static auto select_entry(
    const Abstract& context,
    View::Bytes route,
    View::Bytes callable_name) -> Option<const Ttx::Model::Callable&> {
  const Abstract& receiver = resolve_route(context, route);
  BAIL_IF(receiver.is<Invalid>());
  const Abstract& selected =
      receiver.resolve_call(context, callable_name).resolve();
  auto callable = selected.select<Ttx::Model::Callable>();
  BAIL_IF(
      !callable || !callable->get_parameters().is_empty() ||
      !callable->get_results().is_empty());
  return *callable;
}

auto Language::Program::parse(
    Cursor& cursor,
    const Documentation& documentation) -> Option<Program&> {
  Token opening = require_text(
      cursor, Code::Type::Addressable, "lifecycle"_view,
      "App lifecycle declaration requires `lifecycle`."_view);
  BAIL_IF(!opening);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App lifecycle declaration requires `=` before its policy."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Type, "Program"_view,
      "This App slice accepts only Program lifecycle."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart, "Program lifecycle requires a `{}` body."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Addressable, "start"_view,
      "Program lifecycle requires one `start` entry."_view));

  Token first = cursor.require(
      Code::Type::Type,
      "Program entry requires one Package member Type route."_view);
  BAIL_IF(!first);
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    last = cursor.require(
        Code::Type::Type,
        "Program entry route requires a Type after `::`."_view);
    BAIL_IF(!last);
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Program entry requires `->` before its Static Callable."_view));
  Token callable = cursor.require(
      Code::Type::Addressable,
      "Program entry requires one Static Callable name."_view);
  BAIL_IF(!callable);

  if (cursor.matches(Code::Type::PackingOp) ||
      cursor.matches(Code::Type::EndStatement)) {
    cursor.consume();
  } else {
    cursor.create_token_error(
        cursor.current(), "Program entry requires a trailing `,` or `;`."_view);
    return {};
  }

  Token closing = cursor.require(
      Code::Type::ScopeEnd,
      "Program lifecycle accepts exactly one `start` entry."_view);
  BAIL_IF(!closing);

  Program& program = cursor.get_arena().construct_from<Program>([&]() {
    return Program(
        documentation, route, callable.caculate_text(cursor.get_source_text()),
        Anchor::create(opening, Span(opening, closing)),
        Anchor::create(callable, Span(first, callable)));
  });
  cursor.get_associations().create(program.get_anchor(), program);
  return program;
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
  if (receiver.is<Invalid>()) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry route `"_view << route
           << "` does not resolve in this Package."_view;
    report.get_hint()
        << "Select one exact Package member or nested public context."_view;
    return False;
  }

  const Abstract& selected =
      receiver.resolve_call(context, callable_name).resolve();
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

  entry = Reference<const Ttx::Model::Callable>(*callable);
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

  entry = Reference<const Ttx::Model::Callable>(*callable);
  return True;
}

auto Language::Program::resolve_context(View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}
