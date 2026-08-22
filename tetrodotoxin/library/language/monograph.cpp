// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/object.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/generics/result.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/r32.hpp"
#include "tetrodotoxin/library/language/types/r64.hpp"
#include "tetrodotoxin/library/language/types/s16.hpp"
#include "tetrodotoxin/library/language/types/s32.hpp"
#include "tetrodotoxin/library/language/types/s64.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/library/language/types/u16.hpp"
#include "tetrodotoxin/library/language/types/u32.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
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
    &domain.construct<Types::U8>(),
    &domain.construct<Types::U16>(),
    &domain.construct<Types::U32>(),
    &domain.construct<Types::U64>(),
    &domain.construct<Types::S8>(),
    &domain.construct<Types::S16>(),
    &domain.construct<Types::S32>(),
    &domain.construct<Types::S64>(),
    &domain.construct<Types::R32>(),
    &domain.construct<Types::R64>(),
    &domain.construct<Generics::Access>(domain, *this),
    &domain.construct<Generics::Fixed>(domain, *this),
    &domain.construct<Generics::Option>(domain, *this),
    &domain.construct<Generics::Object>(domain, *this),
    &domain.construct<Generics::Range>(domain, *this),
    &domain.construct<Generics::Result>(domain, *this),
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

auto Library::Language::Monograph::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Abstract& language,
    Abstract& context) -> Option<Monograph&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Source) ||
      record->is_optional() || !reader.is_complete());

  Archive::Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  BAIL_IF(!documentation);

  Monograph& monograph = arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, *documentation, Anchor::create(Span()), language, context);
  });
  BAIL_IF(
      !monograph.source.restore(contents, profile) || !contents.is_complete());
  return monograph;
}

auto Library::Language::Monograph::parse(Cursor& cursor) -> Bool {
  return source.parse(cursor);
}

auto Library::Language::Monograph::link(Cursor& cursor) -> Bool {
  return source.link(cursor, context);
}

auto Library::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  Bool valid = True;
  for (Count index = 0; index < vocabulary.get_size(); index++) {
    auto entry = vocabulary.get_entry(index);
    Option<Generic&> generic;
    if (entry) {
      generic = entry->value.select<Generic>();
    }
    if (generic) {
      valid &= generic->validate_materializations(cursor);
    }
  }

  return valid && source.finalize(cursor);
}

auto Library::Language::Monograph::link_restored() -> Bool {
  return source.link_restored(context);
}

auto Library::Language::Monograph::finalize_restored() -> Bool {
  return source.finalize_restored();
}

auto Library::Language::Monograph::lower(Llvm::Program& program) const
    -> Option<Llvm::Program&> {
  Bool reserved = source.reserve(program);
  if (!reserved) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering failed while reserving source declarations."_view);
    return {};
  }

  Bool completed = source.complete(program);
  if (!completed) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering failed while completing source declarations."_view);
    return {};
  }

  Bool lowered = source.lower(program);
  if (!lowered) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering failed while emitting source declarations."_view);
  }

  return lowered ? Option<Llvm::Program&>(program) : Option<Llvm::Program&>();
}

auto Library::Language::Monograph::persist(Archive::Writer& writer) const
    -> Bool {
  return source.persist(writer);
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
