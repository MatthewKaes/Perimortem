// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/import.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/documentations/merged.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr Ttx::Model::Layouts::Fluid import_layout;

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

auto Language::Import::acquire(const Ttx::Model::Type& root) -> Bool {
  if (acquired) {
    return &acquired->get() == &root;
  }

  acquired = Reference<const Ttx::Model::Type>(root);
  return True;
}

auto Language::Import::get_acquired() const -> Option<const Ttx::Model::Type&> {
  return acquired.visit(
      []() -> Option<const Ttx::Model::Type&> { return {}; },
      [](const Reference<const Ttx::Model::Type>& selected)
          -> Option<const Ttx::Model::Type&> { return selected.get(); });
}

auto Language::Import::select(Option<Cursor&> cursor) const -> const Abstract& {
  if (!acquired) {
    return Invalid::get_invalid();
  }

  const Abstract* selected = &acquired->get();
  if (route.is_empty()) {
    const Abstract& resolved = selected->resolve();
    return resolved.is<Ttx::Model::Type>() ? resolved : Invalid::get_invalid();
  }

  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes selected_name = route.slice(start, index - start);
    const Abstract& context = selected->resolve();
    if (!context.is<Ttx::Model::Type>()) {
      return Invalid::get_invalid();
    }

    const Abstract& queried = context.resolve_context(selected_name);
    if (queried.is<Invalid>()) {
      return queried;
    }
    if (cursor) {
      cursor->get_associations().create(
          segment_anchor(route_anchor, route, selected_name), queried);
    }
    selected = &queried;

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  if (selected->is<Ttx::Model::Type>() && !selected->is<Language::Import>()) {
    return *selected;
  }

  const Abstract& resolved = selected->resolve();
  return resolved.is<Ttx::Model::Type>() ? resolved : Invalid::get_invalid();
}

auto Language::Import::validate(Cursor& cursor) -> Bool {
  const Abstract& selected = select(cursor);
  if (selected.is<Invalid>()) {
    auto report = cursor.create_report(expression_anchor);
    report << "Import Type expression `"_view
           << (kind == Kind::Source ? "source("_view : "package("_view)
           << locator << ")"_view;
    if (!route.is_empty()) {
      report << "::"_view << route;
    }
    report << "` did not resolve."_view;
    report.get_hint()
        << "Publish every selected Type before importing this source."_view;
    return False;
  }

  const Documentation& target_documentation = selected.get_documentation();
  if (local_documentation.is_empty()) {
    visible_documentation = target_documentation;
  } else if (target_documentation.is_empty()) {
    visible_documentation = local_documentation;
  } else if (!visible_documentation) {
    visible_documentation =
        domain.construct<Ttx::Model::Documentations::Merged>(
            local_documentation, target_documentation);
  }
  return True;
}

auto Language::Import::validate_restored() -> Bool {
  const Abstract& selected = select({});
  if (selected.is<Invalid>()) {
    return False;
  }

  const Documentation& target_documentation = selected.get_documentation();
  visible_documentation = local_documentation.is_empty() ? target_documentation
                                                         : local_documentation;
  return True;
}

auto Language::Import::resolve() const -> const Abstract& {
  return select({});
}

auto Language::Import::resolve_context(View::Bytes selected) const
    -> const Abstract& {
  const Abstract& target = resolve();
  return target.is<Invalid>() ? target : target.resolve_context(selected);
}

auto Language::Import::resolve_access(
    const Abstract& host,
    View::Bytes selected) const -> const Abstract& {
  const Abstract& target = resolve();
  return target.is<Invalid>() ? target : target.resolve_access(host, selected);
}

auto Language::Import::resolve_call(const Abstract& host, View::Bytes selected)
    const -> const Abstract& {
  const Abstract& target = resolve();
  return target.is<Invalid>() ? target : target.resolve_call(host, selected);
}

auto Language::Import::get_layout() const -> const Layout& {
  const Abstract& target = resolve();
  auto type = target.select<Ttx::Model::Type>();
  return type ? type->get_layout() : import_layout;
}

auto Language::Import::get_documentation() const -> const Documentation& {
  return visible_documentation.visit(
      [&]() -> const Documentation& { return local_documentation; },
      [](const Documentation& selected) -> const Documentation& {
        return selected;
      });
}
