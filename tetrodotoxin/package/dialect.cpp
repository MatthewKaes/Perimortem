// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/dialect.hpp"

#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/language/source.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Package::Dialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& interpretation_context)
    -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& diagnostics =
      domain.construct<Tetrodotoxin::Language::Diagnostics>(domain);
  return interpret(
      domain, cursor, documentation, source_anchor, diagnostics,
      interpretation_context);
}

auto Package::Dialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Tetrodotoxin::Language::Diagnostics& diagnostics,
    Abstract&) -> Option<Tetrodotoxin::Language::Monograph&> {
  // Package produces a Monograph rather than a synthetic source Type, so it
  // has no semantic owner for the source envelope Anchor.
  (void)source_anchor;
  Managed::Vector<Language::Dependency> dependencies(domain);
  Managed::Vector<Span> dependency_spans(domain);
  Managed::Vector<Language::Source> sources(domain);
  Bool failed = False;
  Bool source_region = False;

  // Consume complete statements until Terminal. Each failed statement reaches
  // a synchronizing terminator before the loop continues, so later independent
  // diagnostics remain observable without risking a stalled Cursor.
  while (!cursor.matches(Code::Type::Terminal)) {
    Token statement = cursor.current();
    switch (cursor.get_code().get_type()) {
    case Code::Type::Resolve: {
      Span dependency_span;
      auto dependency = Language::Dependency::parse(cursor, dependency_span);
      if (!dependency) {
        failed = True;
        continue;
      }

      if (source_region) {
        cursor.create_token_error(
            statement,
            "Resolve statements must precede every Source statement."_view);
        failed = True;
        continue;
      }

      if (dependencies.get_view().contains(
              [&](const Language::Dependency& existing) {
                return existing.get_local_name() ==
                       dependency->get_local_name();
              })) {
        cursor.create_token_error(
            statement,
            "Duplicate Dependency local alias in this Package."_view);
        failed = True;
      }

      // Dependency owns complete statement consumption and exposes only its
      // lexical bounds beside the durable request. Pair them after parsing so
      // recovery cannot leave provenance behind.
      dependencies.insert(*dependency);
      dependency_spans.insert(dependency_span);
      continue;
    }

    case Code::Type::Source: {
      source_region = True;
      Span source_span;
      auto source = Language::Source::parse(domain, cursor, source_span);
      if (!source) {
        failed = True;
        continue;
      }

      // Source owns complete statement consumption, so this range includes the
      // terminating Token. The collision belongs to the whole binding rather
      // than only its opening keyword or semantic name.
      if (dependencies.get_view().contains(
              [&](const Language::Dependency& dependency) {
                return dependency.get_local_name() == source->get_local_name();
              })) {
        cursor.create_expression_error(
            source_span,
            "Source semantic name collides with a Dependency local alias in "
            "this Package."_view);
        failed = True;
      }

      if (sources.get_view().contains([&](const Language::Source& existing) {
            return existing.get_local_name() == source->get_local_name();
          })) {
        cursor.create_token_error(
            statement, "Duplicate Source semantic name in this Package."_view);
        failed = True;
      }

      if (sources.get_view().contains([&](const Language::Source& existing) {
            return existing.get_source_path() == source->get_source_path();
          })) {
        cursor.create_token_error(
            statement,
            "Duplicate normalized Source path in this Package."_view);
        failed = True;
      }

      sources.insert(*source);
      continue;
    }

    default:
      cursor.create_token_error(
          statement,
          "Package bodies contain only `resolve` and `source` statements."_view);
      cursor.recover_to_statement();
      failed = True;
      break;
    }
  }

  // A Package without one complete Source has no semantic member inventory.
  if (sources.is_empty()) {
    cursor.create_error(
        "Package requires at least one complete Source statement."_view);
    failed = True;
  }

  // Publish no graph identity until the complete Package transaction is valid.
  if (failed) {
    return {};
  }

  // Both inventories grow in the same statement branch, but Monograph owns
  // the invariant so another authored producer cannot publish a partial pair.
  auto monograph = Language::Monograph::create_authored(
      domain, documentation, diagnostics, dependencies, dependency_spans,
      sources);
  if (!monograph) {
    return {};
  }

  return *monograph;
}
