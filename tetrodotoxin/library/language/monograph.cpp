// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_16.hpp"
#include "tetrodotoxin/library/language/types/signed_32.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_32.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Library::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Documentation& documentation,
    const Anchor& source_anchor,
    const Abstract& language,
    Abstract& context)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      vocabulary(domain),
      source(
          Types::Source::create_synthetic(
              domain,
              documentation,
              *this,
              source_anchor)) {
  // Each Library root owns the identities that give its source meaning. Keeping
  // the vocabulary beside the graph makes lookup and materialization observe
  // the same objects without a second Type inventory.
  Abstract* identities[] = {
    &domain.construct<Types::Boolean>(),
    &domain.construct<Types::Unsigned_8>(),
    &domain.construct<Types::Unsigned_16>(),
    &domain.construct<Types::Unsigned_32>(),
    &domain.construct<Types::Unsigned_64>(),
    &domain.construct<Types::Signed_8>(),
    &domain.construct<Types::Signed_16>(),
    &domain.construct<Types::Signed_32>(),
    &domain.construct<Types::Signed_64>(),
    &domain.construct<Types::Real_32>(),
    &domain.construct<Types::Real_64>(),
    &domain.construct<Generics::Access>(domain, *this),
    &domain.construct<Generics::Fixed>(domain, *this),
    &domain.construct<Generics::Option>(domain, *this),
    &domain.construct<Generics::Range>(domain, *this),
    &domain.construct<Generics::View>(domain, *this),
  };
  for (Abstract* identity : identities) {
    vocabulary.launder(identity->get_name(), *identity);
  }
}

auto Library::Language::Monograph::create_authored(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    const Abstract& language,
    Abstract& context) -> Monograph& {
  Allocator::Arena& arena = cursor.get_arena();
  return arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(arena, documentation, source_anchor, language, context);
  });
}

auto Library::Language::Monograph::parse(Cursor& cursor) -> Bool {
  return source.parse(cursor);
}

auto Library::Language::Monograph::link(Cursor& cursor) -> Bool {
  return source.link(cursor, context);
}

auto Library::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  return source.finalize(cursor);
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // Source and Foreign are the two reserved authored contexts. Ordinary
  // declarations remain in Source while root vocabulary and using contexts
  // answer only names that those owned contexts leave unresolved.
  if (route == "source"_view) {
    return source;
  }
  if (route == "foreign"_view && source.get_foreign().is_authored()) {
    return source.get_foreign();
  }

  const Abstract& authored = source.resolve_local(route);
  if (!authored.is<Invalid>()) {
    return authored;
  }

  const Abstract& root = resolve_root_context(route);
  if (!root.is<Invalid>()) {
    return root;
  }
  return source.resolve_imports(route);
}

auto Library::Language::Monograph::resolve_root_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& intrinsic = vocabulary.visit(
      route,
      [](const Abstract& selected) -> const Abstract& { return selected; },
      []() -> const Abstract& { return Invalid::get_invalid(); });
  if (!intrinsic.is<Invalid>()) {
    return intrinsic;
  }

  return Tetrodotoxin::Language::Monograph::resolve_context(route);
}

auto Library::Language::Monograph::resolve_access(
    const Abstract& host,
    View::Bytes route) const -> const Abstract& {
  return source.resolve_type_access(host, route, Model::Type::Access::Static);
}

auto Library::Language::Monograph::resolve_call(
    const Abstract& host,
    View::Bytes route) const -> const Abstract& {
  return source.resolve_type_call(host, route, Model::Type::Access::Static);
}
