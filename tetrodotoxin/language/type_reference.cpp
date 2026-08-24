// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/type_reference.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Language::TypeReference::resolve(Cursor& cursor, const Abstract& context)
    const -> Option<const Ttx::Model::Type&> {
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
      auto report = cursor.create_report(anchor);
      report << "Type route `"_view << route
             << "` could not resolve segment "_view << U64(segment) << "."_view;
      report.get_hint()
          << "Publish that Type in the selected semantic context."_view;
      return {};
    }
    selected = candidate;
    segment++;

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  auto type = selected->select<Ttx::Model::Type>();
  if (!type) {
    auto report = cursor.create_report(anchor);
    report << "Route `"_view << route << "` does not select one Type."_view;
    report.get_hint()
        << "Select one value Type rather than a contextual declaration."_view;
    return {};
  }

  cursor.get_associations().create(anchor, *type);
  return *type;
}
