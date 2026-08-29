// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/import.hpp"

#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Language::Import::acquire(const Ttx::Model::Type& root) -> Bool {
  if (acquired) {
    return acquired == &root;
  }

  acquired = &root;
  target = &root;
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }
    View::Bytes name = route.slice(start, index - start);
    if (!name.is_empty()) {
      target = &Language::Reference::create(domain, *target, name);
    }
    if (separator) {
      index++;
      start = index + 1;
    }
  }
  return True;
}

auto Language::Import::get_acquired() const -> Option<const Ttx::Model::Type&> {
  return acquired ? Option<const Ttx::Model::Type&>(*acquired)
                  : Option<const Ttx::Model::Type&>();
}

auto Language::Import::select_target() const -> const Abstract& {
  if (!target) {
    return Unknown::get_unknown();
  }

  const Abstract& resolved = target->resolve();
  return resolved.is<Ttx::Model::Type>() ? resolved : None::get_none();
}

auto Language::Import::validate(Cursor& cursor) -> Bool {
  const Abstract& selected = select_target();
  if (selected.is<Unknown>() || selected.is<None>()) {
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

  cursor.get_associations().create(route_anchor, selected);
  return True;
}

auto Language::Import::validate_restored() -> Bool {
  const Abstract& selected = select_target();
  if (selected.is<Unknown>() || selected.is<None>()) {
    return False;
  }

  return True;
}

auto Language::Import::resolve() const -> const Abstract& {
  return select_target();
}

auto Language::Import::get_documentation() const -> const Documentation& {
  return local_documentation;
}
