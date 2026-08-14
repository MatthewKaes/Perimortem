// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_16.hpp"
#include "tetrodotoxin/library/language/types/signed_32.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_32.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
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
static constexpr Library::Language::Types::Descriptor descriptor;

static constexpr Static::Vector<Pair<View::Bytes, const Abstract*>, 11>
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
    }};

using Intrinsics = Table<const Abstract*, intrinsic_source>;

auto Library::Dialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& interpretation_context)
    -> Option<Tetrodotoxin::Language::Monograph&> {
  auto shared_materializations = materializations_for(domain, cursor);
  if (!shared_materializations) {
    return {};
  }

  auto& monograph = Library::Language::Monograph::create_authored(
      domain, documentation, source_anchor, *this, interpretation_context);
  Bool parsed = monograph.get_source().parse(cursor);
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

auto Library::Dialect::get_materializations() const
    -> Language::Materializations& {
  // A Library Monograph can exist only after interpret() binds this edge. The
  // append only writer remains logically shared by const semantic queries even
  // while a first request publishes a canonical generated Type.
  return *materializations;
}

auto Library::Dialect::resolve_intrinsic(View::Bytes name) const
    -> const Abstract& {
  // A formula is an intrinsic identity too, but its function local singleton
  // cannot participate in the constexpr Type table. Asking each formula
  // owner preserves the exact object used by Materializations instead of
  // creating a lookup only copy with a different canonical key.
  if (name == Language::Generics::Access::name) {
    return Language::Generics::Access::get_formula();
  }
  if (name == Language::Generics::Fixed::name) {
    return Language::Generics::Fixed::get_formula();
  }
  if (name == Language::Generics::Option::name) {
    return Language::Generics::Option::get_formula();
  }
  if (name == Language::Generics::Range::name) {
    return Language::Generics::Range::get_formula();
  }
  if (name == Language::Generics::View::name) {
    return Language::Generics::View::get_formula();
  }

  // Immutable Library Types keep one binary wide identity. The packed table
  // preserves their constexpr addresses without copying them into each
  // installed Dialect allowing the compiler to optimize a lot of the lookup.
  return *Intrinsics::find_or_default(name, &Invalid::get_invalid());
}

static auto create_default(
    Allocator::Arena& domain,
    const Ttx::Model::Type& type,
    Dynamic::Vector<const Ttx::Model::Type*>& path)
    -> Option<Library::Language::Model::Pack&> {
  // Defaults are attached to the exact Library identity selected by the
  // caller. Category proof alone would let an unrelated language inherit a
  // value policy that belongs only to this Dialect.
  if (&type == &Library::Dialect::get_bool()) {
    return Library::Language::Constants::False::create_synthetic(
        domain, Library::Dialect::get_bool());
  }

  if (&type == &Library::Dialect::get_unsigned_8() ||
      &type == &Library::Dialect::get_unsigned_16() ||
      &type == &Library::Dialect::get_unsigned_32() ||
      &type == &Library::Dialect::get_unsigned_64()) {
    return Library::Language::Constants::Unsigned::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Unsigned&>(type), 0);
  }

  if (&type == &Library::Dialect::get_signed_8() ||
      &type == &Library::Dialect::get_signed_16() ||
      &type == &Library::Dialect::get_signed_32() ||
      &type == &Library::Dialect::get_signed_64()) {
    return Library::Language::Constants::Signed::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Signed&>(type), 0);
  }

  if (&type == &Library::Dialect::get_real_32() ||
      &type == &Library::Dialect::get_real_64()) {
    return Library::Language::Constants::Real::create_synthetic(
        domain, static_cast<const Ttx::Model::Types::Real&>(type), 0.0);
  }

  auto view = type.select<Library::Language::Types::View>();
  if (view) {
    return Library::Language::Constants::Bytes::create_synthetic(
        domain, type, {});
  }

  if (type.is<Library::Language::Types::Access>()) {
    return Library::Language::Constants::Bytes::create_synthetic(
        domain, type, {});
  }

  // These domains own a complete empty or zero state without asking their
  // element Type for a value. Option therefore also terminates recursive
  // default discovery before a payload construction begins.
  auto enumeration = type.select<Library::Language::Types::Enumeration>();
  if (enumeration) {
    auto storage = enumeration->get_storage_type();
    BAIL_IF(
        !storage || (!storage->is<Ttx::Model::Types::Signed>() &&
                     !storage->is<Ttx::Model::Types::Unsigned>()));
    return Library::Language::Constants::Enumeration::create_synthetic(
        domain, *enumeration, 0);
  }

  auto range = type.select<Library::Language::Types::Range>();
  if (range) {
    return Library::Language::Constants::Range::create_synthetic(
        domain, *range);
  }

  auto option = type.select<Library::Language::Types::Option>();
  if (option) {
    return Library::Language::Constants::Option::create_absent(domain, *option);
  }

  // Fixed preserves its promised extent as real child Packs. Materialization
  // admits only a positive extent, so every default contains real value flow.
  auto fixed = type.select<Library::Language::Types::Fixed>();
  if (fixed) {
    BAIL_IF(
        fixed->get_extent() == 0 ||
        fixed->get_extent() > Unsigned_64(Count(-1)) || path.contains(&type));

    path.insert(&type);
    Managed::Vector<Reference<Library::Language::Model::Pack>> values(domain);
    values.reset(Count(fixed->get_extent()));
    for (Count index = 0; index < Count(fixed->get_extent()); index++) {
      auto value = create_default(domain, fixed->get_element_type(), path);
      if (!value) {
        path.remove(path.get_size() - 1);
        return {};
      }
      values.insert(*value);
    }
    path.remove(path.get_size() - 1);
    return Library::Language::Expressions::Initializer::create_synthetic(
        domain, type, values.get_view());
  }

  // Structure and Object reuse the exact state Field inventory and authored
  // order. Initializers remain their real value flow, while an omitted Field
  // recurses through this same owner and shares the active cycle path.
  auto structure = type.select<Library::Language::Types::Structure>();
  if (structure) {
    BAIL_IF(structure->get_layout().is_empty());
    BAIL_IF(path.contains(&type));

    path.insert(&type);
    Managed::Vector<Reference<Library::Language::Model::Pack>> values(domain);
    values.reset(structure->get_layout().get_size());
    for (const Reference<Abstract>& candidate : structure->get_addressables()) {
      auto field = candidate.get().select<Library::Language::Field>();
      if (!field || field->get_writability() !=
                        Library::Language::Writability::Internal) {
        continue;
      }

      auto initializer = field->get_initializer();
      if (initializer) {
        values.insert(
            const_cast<Library::Language::Model::Pack&>(*initializer));
        continue;
      }

      auto value = create_default(domain, field->get_type(), path);
      if (!value) {
        path.remove(path.get_size() - 1);
        return {};
      }
      values.insert(*value);
    }
    path.remove(path.get_size() - 1);
    return Library::Language::Expressions::Initializer::create_synthetic(
        domain, type, values.get_view());
  }

  return {};
}

auto Library::Dialect::create_default(
    Allocator::Arena& domain,
    const Ttx::Model::Type& type) -> Option<Language::Model::Pack&> {
  Dynamic::Vector<const Ttx::Model::Type*> path;
  return ::create_default(domain, type, path);
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

auto Library::Dialect::get_descriptor() -> const Language::Types::Descriptor& {
  return descriptor;
}
