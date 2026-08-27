// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto is_resource_route(View::Bytes route) -> Bool {
  return route.get_size() >= 3 && route[0] == '$' && route[1] == '[' &&
         route[route.get_size() - 1] == ']';
}

auto Package::Language::Monograph::create_authored(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context,
    const Abstract& library_language) -> Monograph& {
  return arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, documentation, source_anchor, context,
        library_language, {}, False);
  });
}

auto Package::Language::Monograph::create_synthetic(
    Allocator::Arena& arena,
    const Abstract& language,
    Abstract& context,
    const Abstract& library_language,
    View::Vector<Reference<Package::Resource>> restored_resources)
    -> Monograph& {
  Monograph& monograph = arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, Documentation::get_empty(), Anchor::create(Span()),
        context, library_language, restored_resources, True);
  });
  return monograph;
}

Package::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context,
    const Abstract& library_language,
    View::Vector<Reference<Package::Resource>> restored_resources,
    Bool resources_sealed)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      resources(domain, restored_resources, resources_sealed),
      library(
          Library::Language::Monograph::create_authored(
              domain,
              documentation,
              source_anchor,
              library_language,
              *this)) {}

auto Package::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // The delimiters reserve one complete resource route. Malformed or partial
  // spellings continue through exact Package lookup so this branch never
  // becomes a second Embedded parser.
  if (is_resource_route(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }

  const Abstract& exported = library.resolve_local_context(route);
  if (!exported.is<Invalid>()) {
    return exported;
  }

  return Tetrodotoxin::Language::Monograph::resolve_context(route);
}

auto Package::Language::Monograph::resolve_lexical_context(
    View::Bytes route) const -> const Abstract& {
  if (is_resource_route(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }

  const Abstract& selected = library.resolve_lexical_context(route);
  return selected.is<Invalid>()
             ? Tetrodotoxin::Language::Monograph::resolve_lexical_context(route)
             : selected;
}

auto Package::Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  auto outer = Tetrodotoxin::Language::Monograph::get_layer(requested);
  if (outer) {
    return *outer;
  }
  return &requested == &library.get_language()
             ? Option<const Tetrodotoxin::Language::Monograph&>(library)
             : Option<const Tetrodotoxin::Language::Monograph&>();
}

auto Package::Language::Monograph::link(Cursor& cursor) -> Bool {
  return library.link(cursor);
}

auto Package::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  return library.finalize(cursor);
}

auto Package::Language::Monograph::link_restored() -> Bool {
  return library.link_restored();
}

auto Package::Language::Monograph::finalize_restored() -> Bool {
  return library.finalize_restored();
}

auto Package::Language::Monograph::get_name() const -> View::Bytes {
  return "Package"_view;
}

auto Package::Language::Monograph::get_resources() -> Package::Resources& {
  return resources;
}

auto Package::Language::Monograph::get_resources() const
    -> const Package::Resources& {
  return resources;
}
