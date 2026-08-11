// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_16.hpp"
#include "tetrodotoxin/library/language/types/signed_32.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_32.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/library/language/types/void.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr Library::Language::Types::Boolean boolean;
static constexpr Library::Language::Types::Unsigned_8 unsigned_8;
static constexpr Library::Language::Types::Unsigned_16 unsigned_16;
static constexpr Library::Language::Types::Unsigned_32 unsigned_32;
static constexpr Library::Language::Types::Unsigned_64 unsigned_64;
static constexpr Library::Language::Types::Signed_8 signed_8;
static constexpr Library::Language::Types::Signed_16 signed_16;
static constexpr Library::Language::Types::Signed_32 signed_32;
static constexpr Library::Language::Types::Signed_64 signed_64;
static constexpr Library::Language::Types::Real_32 real_32;
static constexpr Library::Language::Types::Real_64 real_64;
static constexpr Library::Language::Types::Void void_type;

static constexpr Static::Vector<Pair<View::Bytes, const Abstract*>, 12>
    intrinsic_source = {{
      Pair<View::Bytes, const Abstract*>{boolean.get_name(), &boolean},
      {unsigned_8.get_name(), &unsigned_8},
      {unsigned_16.get_name(), &unsigned_16},
      {unsigned_32.get_name(), &unsigned_32},
      {unsigned_64.get_name(), &unsigned_64},
      {signed_8.get_name(), &signed_8},
      {signed_16.get_name(), &signed_16},
      {signed_32.get_name(), &signed_32},
      {signed_64.get_name(), &signed_64},
      {real_32.get_name(), &real_32},
      {real_64.get_name(), &real_64},
      {void_type.get_name(), &void_type},
    }};

using Intrinsics = Table<const Abstract*, intrinsic_source>;

auto Library::Dialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& interpretation_context)
    -> Option<Tetrodotoxin::Language::Monograph&> {
  auto shared_materializations = materializations_for(domain, cursor);
  if (!shared_materializations) {
    return {};
  }

  auto& monograph = Library::Language::Monograph::create_authored(
      domain, documentation, *this, interpretation_context,
      *shared_materializations);
  auto source =
      monograph.get_source().select<Library::Language::Types::Source>();
  if (!source) {
    cursor.create_token_error(
        "Library sources require one exact synthetic Source Type."_view);
    return {};
  }

  Bool parsed = source->parse(cursor);
  if (!parsed) {
    return {};
  }

  return monograph;
}

auto Library::Dialect::materializations_for(
    Allocator::Arena& domain,
    Cursor& cursor) -> Option<Language::Materializations&> {
  // One installed Dialect belongs to one Workspace Arena. Reusing its writer
  // keeps equal generated Types exact across every Monograph in that island.
  if (materializations) {
    if (&*materialization_domain != &domain) {
      cursor.create_token_error(
          "One installed Library Dialect cannot span two graph Arenas."_view);
      return {};
    }

    return *materializations;
  }

  auto& created = domain.construct<Language::Materializations>(domain);
  materialization_domain = domain;
  materializations = created;
  return created;
}

auto Library::Dialect::resolve_intrinsic(View::Bytes name) const
    -> const Abstract& {
  // Immutable Library Types keep one binary wide identity. The packed table
  // preserves their constexpr addresses without copying them into each
  // installed Dialect allowing the compiler to optimize a lot of the lookup.
  return *Intrinsics::find_or_default(name, &Invalid::get_invalid());
}

auto Library::Dialect::create_default(
    Allocator::Arena& domain,
    const Ttx::Model::Type& type) -> Option<Language::Constant&> {
  // Defaults are attached to the exact Library identity selected by the
  // caller. Category proof alone would let an unrelated language inherit a
  // value policy that belongs only to this Dialect.
  if (&type == &get_bool()) {
    return Language::Constants::False::create_synthetic(domain, get_bool());
  }

  if (&type == &get_unsigned_8() || &type == &get_unsigned_16() ||
      &type == &get_unsigned_32() || &type == &get_unsigned_64()) {
    return Language::Constants::Unsigned::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Unsigned&>(type), 0);
  }

  if (&type == &get_signed_8() || &type == &get_signed_16() ||
      &type == &get_signed_32() || &type == &get_signed_64()) {
    return Language::Constants::Signed::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Signed&>(type), 0);
  }

  if (&type == &get_real_32() || &type == &get_real_64()) {
    return Language::Constants::Real::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Real&>(type), 0.0);
  }

  auto view = type.select<Language::Types::View>();
  if (view) {
    return Language::Constants::Bytes::create_synthetic(domain, type, {});
  }

  return {};
}

auto Library::Dialect::get_bool() -> const Ttx::Model::Types::Flag& {
  return boolean;
}

auto Library::Dialect::get_unsigned_8() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_8;
}

auto Library::Dialect::get_unsigned_16() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_16;
}

auto Library::Dialect::get_unsigned_32() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_32;
}

auto Library::Dialect::get_unsigned_64() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_64;
}

auto Library::Dialect::get_signed_8() -> const Ttx::Model::Types::Signed& {
  return signed_8;
}

auto Library::Dialect::get_signed_16() -> const Ttx::Model::Types::Signed& {
  return signed_16;
}

auto Library::Dialect::get_signed_32() -> const Ttx::Model::Types::Signed& {
  return signed_32;
}

auto Library::Dialect::get_signed_64() -> const Ttx::Model::Types::Signed& {
  return signed_64;
}

auto Library::Dialect::get_real_32() -> const Ttx::Model::Types::Real& {
  return real_32;
}

auto Library::Dialect::get_real_64() -> const Ttx::Model::Types::Real& {
  return real_64;
}

auto Library::Dialect::get_void() -> const Ttx::Model::Type& {
  return void_type;
}
