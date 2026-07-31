// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/retention.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Environment::Retention::Entry::Entry(
    Language::Dialect::Monograph& monograph,
    Option<Origin> origin)
    : monograph(monograph),
      origin_path(),
      origin_body(),
      origin_span(),
      has_origin(False) {
  origin.visit(
      []() {},
      [&](const Origin& authored) {
        origin_path = authored.get_path();
        origin_body = authored.get_body();
        origin_span = authored.get_span();
        has_origin = True;
      });
}

auto Environment::Retention::Entry::get_monograph() const
    -> Language::Dialect::Monograph& {
  return monograph;
}

auto Environment::Retention::Entry::get_origin() const -> Option<Origin> {
  if (!has_origin) {
    return {};
  }

  return Origin(origin_path, origin_body, origin_span);
}

Environment::Retention::Retention(Allocator::Arena& arena)
    : entries(arena), next_to_complete(0) {}

Environment::Retention::~Retention() {
  for (Count i = 0; i < entries.get_size(); i++) {
    entries[i].get_monograph().~Monograph();
  }
}

auto Environment::Retention::retain(
    Language::Dialect::Monograph& monograph,
    Option<Origin> origin) -> void {
  for (Count i = 0; i < entries.get_size(); i++) {
    if (&entries[i].get_monograph() == &monograph) {
      return;
    }
  }

  // One entry keeps identity, discovery order, and optional source provenance
  // aligned even when a restore attempt retains only part of its graph.
  Entry retained(monograph, origin);
  entries.insert(retained);
}

auto Environment::Retention::get_size() const -> Count {
  return entries.get_size();
}

auto Environment::Retention::get_monograph(Count index) const
    -> Language::Dialect::Monograph& {
  return entries.at(index).get_monograph();
}

auto Environment::Retention::get_origin(Count index) const -> Option<Origin> {
  return entries.at(index).get_origin();
}

auto Environment::Retention::find_origin(
    const Language::Dialect::Monograph& monograph) const -> Option<Origin> {
  for (Count i = 0; i < entries.get_size(); i++) {
    const Entry& retained = entries.at(i);
    if (&retained.get_monograph() == &monograph) {
      return retained.get_origin();
    }
  }

  return {};
}

auto Environment::Retention::complete(Errors& errors) -> Bool {
  // Advance immediately after every call. A failed concrete hook remains part
  // of the completed prefix and cannot run again during a later import.
  Bool completed = True;
  while (next_to_complete < entries.get_size()) {
    Entry& retained = entries[next_to_complete];
    Bool passed = retained.get_monograph().post_pass();
    next_to_complete++;
    if (passed) {
      continue;
    }

    // Binary restored state has no authored Origin. Its concrete owner keeps
    // graph detail in its own log, while authored state receives a Report from
    // the exact retained bytes.
    retained.get_origin().visit(
        []() {},
        [&](const Origin& origin) {
          Errors::Report report(
              errors, origin.get_path(), origin.get_body(), origin.get_span());
          report << "Semantic completion failed for "_view
                 << retained.get_monograph().get_name() << '.';
        });
    completed = False;
  }

  return completed;
}
