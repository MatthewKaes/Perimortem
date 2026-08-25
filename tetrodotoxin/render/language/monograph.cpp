// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Monograph::create(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context) -> Monograph& {
  return arena.construct_from<Monograph>(
      [&]() { return Monograph(arena, language, documentation, context); });
}

auto Language::Monograph::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_addressable(declaration, visibility);
}

auto Language::Monograph::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_callable(declaration, visibility);
}

auto Language::Monograph::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_type(declaration, visibility);
}

auto Language::Monograph::link(Cursor& cursor) -> Bool {
  return declarations.link(cursor, *this);
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  finalized = declarations.is_linked();
  return finalized;
}

auto Language::Monograph::link_restored() -> Bool {
  return declarations.link_restored(*this);
}

auto Language::Monograph::finalize_restored() -> Bool {
  finalized = declarations.is_linked();
  return finalized;
}

auto Language::Monograph::resolve_context(View::Bytes name) const
    -> const Abstract& {
  const Abstract& local = declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Public);
  return local.is<Invalid>()
             ? Tetrodotoxin::Language::Monograph::resolve_context(name)
             : local;
}

auto Language::Monograph::resolve_local_context(View::Bytes name) const
    -> const Abstract& {
  return declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Private);
}

auto Language::Monograph::resolve_access(const Abstract&, View::Bytes name)
    const -> const Abstract& {
  return declarations.resolve_addressable(
      name, Tetrodotoxin::Language::Visibility::Public);
}

auto Language::Monograph::resolve_call(const Abstract&, View::Bytes name) const
    -> const Abstract& {
  return declarations.resolve_callable(
      name, Tetrodotoxin::Language::Visibility::Public);
}
