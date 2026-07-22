// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/builtins.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "ttx/model/types/bool.hpp"
#include "ttx/model/types/real_32.hpp"
#include "ttx/model/types/real_64.hpp"
#include "ttx/model/types/signed_16.hpp"
#include "ttx/model/types/signed_32.hpp"
#include "ttx/model/types/signed_64.hpp"
#include "ttx/model/types/signed_8.hpp"
#include "ttx/model/types/unsigned_16.hpp"
#include "ttx/model/types/unsigned_32.hpp"
#include "ttx/model/types/unsigned_64.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Model;

static constexpr Types::Boolean boolean;
static constexpr Types::Unsigned_8 unsigned_8;
static constexpr Types::Unsigned_16 unsigned_16;
static constexpr Types::Unsigned_32 unsigned_32;
static constexpr Types::Unsigned_64 unsigned_64;
static constexpr Types::Signed_8 signed_8;
static constexpr Types::Signed_16 signed_16;
static constexpr Types::Signed_32 signed_32;
static constexpr Types::Signed_64 signed_64;
static constexpr Types::Real_32 real_32;
static constexpr Types::Real_64 real_64;

using Builtin = Option<const Type&>;
using Entry = Pair<View::Bytes, Builtin>;

// Count is Tetrodotoxin's ordinary size alias. Parsing returns the resolved
// Unsigned_64 Type identity rather than manufacturing a second alias Type.
static constexpr Static::Vector<Entry, 12> builtin_source = {{
  {"Bool"_view, boolean},
  {"Count"_view, unsigned_64},
  {"Real_32"_view, real_32},
  {"Real_64"_view, real_64},
  {"Signed_8"_view, signed_8},
  {"Signed_16"_view, signed_16},
  {"Signed_32"_view, signed_32},
  {"Signed_64"_view, signed_64},
  {"Unsigned_8"_view, unsigned_8},
  {"Unsigned_16"_view, unsigned_16},
  {"Unsigned_32"_view, unsigned_32},
  {"Unsigned_64"_view, unsigned_64},
}};

using BuiltinTable = Table<Builtin, builtin_source>;

auto Tetrodotoxin::Parser::Builtins::find(View::Bytes name) -> Builtin {
  return BuiltinTable::find_or_default(name, none);
}
