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

namespace Tetrodotoxin::Parser {

using Builtin = Perimortem::Utility::Option<const Ttx::Model::Type&>;
using Entry = Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Builtin>;

static constexpr Ttx::Model::Types::Boolean boolean;
static constexpr Ttx::Model::Types::Unsigned_8 unsigned_8;
static constexpr Ttx::Model::Types::Unsigned_16 unsigned_16;
static constexpr Ttx::Model::Types::Unsigned_32 unsigned_32;
static constexpr Ttx::Model::Types::Unsigned_64 unsigned_64;
static constexpr Ttx::Model::Types::Signed_8 signed_8;
static constexpr Ttx::Model::Types::Signed_16 signed_16;
static constexpr Ttx::Model::Types::Signed_32 signed_32;
static constexpr Ttx::Model::Types::Signed_64 signed_64;
static constexpr Ttx::Model::Types::Real_32 real_32;
static constexpr Ttx::Model::Types::Real_64 real_64;

// Built in types used by common dialects (such as Library) can be optimized to
// have a global override context by simply having a look up type intercept Type
// parsing and abstract mapping.
static constexpr Perimortem::Core::Static::Vector<Entry, 12> builtin_source = {{
  {boolean.get_name(), boolean},
  {real_32.get_name(), real_32},
  {real_64.get_name(), real_64},
  {signed_8.get_name(), signed_8},
  {signed_16.get_name(), signed_16},
  {signed_32.get_name(), signed_32},
  {signed_64.get_name(), signed_64},
  {unsigned_8.get_name(), unsigned_8},
  {unsigned_16.get_name(), unsigned_16},
  {unsigned_32.get_name(), unsigned_32},
  {unsigned_64.get_name(), unsigned_64},
  // Count is Tetrodotoxin's ordinary size alias. Parsing returns the resolved
  // Unsigned_64 Type identity rather than manufacturing a second alias Type.
  {"Count"_view, unsigned_64},
}};

using BuiltinTable = Perimortem::Utility::Table<Builtin, builtin_source>;

auto Builtins::find(Perimortem::Core::View::Bytes name) -> Builtin {
  return BuiltinTable::find_or_default(name, Perimortem::Utility::none);
}

}  // namespace Tetrodotoxin::Parser
