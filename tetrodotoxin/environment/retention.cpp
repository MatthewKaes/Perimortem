// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/retention.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Environment::Retention::Entry::Entry(
    Language::Monograph& monograph,
    Option<Origin> origin)
    : monograph(monograph),
      origin_path(),
      origin_body(),
      origin_span(),
      next_diagnostic(0),
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
    -> Language::Monograph& {
  return monograph;
}

auto Environment::Retention::Entry::get_origin() const -> Option<Origin> {
  BAIL_IF(!has_origin);

  return Origin(origin_path, origin_body, origin_span);
}

auto Environment::Retention::Entry::get_next_diagnostic() const -> Count {
  return next_diagnostic;
}

auto Environment::Retention::Entry::consume_diagnostics(Count count) -> void {
  next_diagnostic = count;
}

Environment::Retention::Retention(Allocator::Arena& arena)
    : entries(arena), range_start(0), range_end(0), stage(Stage::Staging) {}

Environment::Retention::~Retention() {
  for (Count i = 0; i < entries.get_size(); i++) {
    entries[i].get_monograph().~Monograph();
  }
}

auto Environment::Retention::retain(
    Language::Monograph& monograph,
    Option<Origin> origin) -> Bool {
  BAIL_IF(stage == Stage::Linked);

  Bool already_retained = entries.get_view().contains(
      [&](const Entry& entry) { return &entry.get_monograph() == &monograph; });
  if (already_retained) {
    return True;
  }

  // One entry keeps identity, discovery order, and optional source provenance
  // aligned even when a restore attempt retains only part of its graph.
  Entry retained(monograph, origin);
  entries.insert(retained);
  return True;
}

auto Environment::Retention::get_size() const -> Count {
  return entries.get_size();
}

auto Environment::Retention::get_monograph(Count index) const
    -> Language::Monograph& {
  return entries.at(index).get_monograph();
}

auto Environment::Retention::get_origin(Count index) const -> Option<Origin> {
  return entries.at(index).get_origin();
}

auto Environment::Retention::find_origin(
    const Language::Monograph& monograph) const -> Option<Origin> {
  for (Count i = 0; i < entries.get_size(); i++) {
    const Entry& retained = entries.at(i);
    if (&retained.get_monograph() == &monograph) {
      return retained.get_origin();
    }
  }

  return {};
}

auto Environment::Retention::has_staged() const -> Bool {
  return stage == Stage::Staging && range_start < entries.get_size();
}

auto Environment::Retention::awaits_finalize() const -> Bool {
  return stage == Stage::Linked;
}

auto Environment::Retention::render_diagnostics(Errors& errors, Entry& entry)
    -> void {
  View::Vector<Language::Diagnostic> diagnostics =
      entry.get_monograph().get_diagnostics();
  Count first = entry.get_next_diagnostic();
  for (Count i = first; i < diagnostics.get_size(); i++) {
    const Language::Diagnostic& diagnostic = diagnostics.get_data()[i];
    entry.get_origin().visit(
        []() {},
        [&](const Origin& origin) {
          // A restored Monograph owns no source coordinate. Its borrowed Origin
          // inherits the nearest authored dependency boundary for presentation.
          Anchor anchor = diagnostic.get_anchor().visit(
              [&]() { return Anchor::create(origin.get_span()); },
              [](const Anchor& selected) { return selected; });

          Errors::Report report(
              errors, origin.get_path(), origin.get_body(), anchor);
          report << diagnostic.get_message();
          if (!diagnostic.get_hint().is_empty()) {
            report.get_hint() << diagnostic.get_hint();
          }
        });
  }

  entry.consume_diagnostics(diagnostics.get_size());
}

auto Environment::Retention::consume_range() -> void {
  range_start = range_end;
  stage = Stage::Staging;
}

auto Environment::Retention::link(Errors& errors) -> Bool {
  BAIL_IF(stage != Stage::Staging || !has_staged());

  // Freeze before the first hook. A linked Monograph cannot grow this range or
  // make a later discovery escape the all links before finalization barrier.
  range_end = entries.get_size();
  stage = Stage::Linked;
  Bool failed = False;
  for (Count i = range_start; i < range_end; i++) {
    Entry& entry = entries[i];
    Bool linked = entry.get_monograph().link();
    render_diagnostics(errors, entry);
    failed |= !linked;
  }

  if (failed) {
    consume_range();
    return False;
  }

  return True;
}

auto Environment::Retention::finalize(Errors& errors) -> Bool {
  BAIL_IF(stage != Stage::Linked);

  Bool failed = False;
  for (Count i = range_start; i < range_end; i++) {
    Entry& entry = entries[i];
    Bool finalized = entry.get_monograph().finalize();
    render_diagnostics(errors, entry);
    failed |= !finalized;
  }

  consume_range();
  return !failed;
}

auto Environment::Retention::abandon() -> void {
  range_end = stage == Stage::Linked ? range_end : entries.get_size();
  consume_range();
}
