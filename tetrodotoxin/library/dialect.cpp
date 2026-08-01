// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
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
      {boolean.get_name(), &boolean},
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
    Abstract&) -> Option<Tetrodotoxin::Language::Dialect::Monograph&> {
  auto& monograph = domain.construct<Library::Language::Monograph>(
      domain, documentation, *this);

  // A Function must be reachable at its final address while its signature
  // builds. Signatures only resolve Dialect Types here, so one pass preserves
  // authored order without retaining discovery state for later declarations.
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& function_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto function = Library::Language::Function::reserve(
        domain, cursor, function_documentation);
    if (!function) {
      return {};
    }

    if (!monograph.bind_function(*function)) {
      cursor.create_token_error(
          "Duplicate Function name in this Library source."_view);
      return {};
    }

    if (!(*function).complete(cursor, monograph)) {
      return {};
    }
  }

  return monograph;
}

auto Library::Dialect::resolve_intrinsic(View::Bytes name) const
    -> const Abstract& {
  // Immutable Library Types keep one binary wide identity. The packed table
  // preserves their constexpr addresses without copying them into each
  // installed Dialect allowing the compiler to optimize a lot of the lookup.
  return *Intrinsics::find_or_default(name, &Invalid::get_invalid());
}
