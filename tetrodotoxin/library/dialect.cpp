// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

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

using namespace Tetrodotoxin::Library;

// Containers for all of the built in type Abstracts.
static constexpr Language::Types::Boolean boolean;
static constexpr Language::Types::Unsigned_8 unsigned_8;
static constexpr Language::Types::Unsigned_16 unsigned_16;
static constexpr Language::Types::Unsigned_32 unsigned_32;
static constexpr Language::Types::Unsigned_64 unsigned_64;
static constexpr Language::Types::Signed_8 signed_8;
static constexpr Language::Types::Signed_16 signed_16;
static constexpr Language::Types::Signed_32 signed_32;
static constexpr Language::Types::Signed_64 signed_64;
static constexpr Language::Types::Real_32 real_32;
static constexpr Language::Types::Real_64 real_64;

// Built in types used by common dialects (such as Library) can be optimized to
// have a global override context by simply having a look up type intercept Type
// parsing and abstract mapping.
static constexpr Perimortem::Core::Static::Vector<
    Perimortem::Utility::
        Pair<Perimortem::Core::View::Bytes, const Ttx::Model::Type*>,
    12>
    standard_source = {{
      {boolean.get_name(), &boolean},
      {real_32.get_name(), &real_32},
      {real_64.get_name(), &real_64},
      {signed_8.get_name(), &signed_8},
      {signed_16.get_name(), &signed_16},
      {signed_32.get_name(), &signed_32},
      {signed_64.get_name(), &signed_64},
      {unsigned_8.get_name(), &unsigned_8},
      {unsigned_16.get_name(), &unsigned_16},
      {unsigned_32.get_name(), &unsigned_32},
      {unsigned_64.get_name(), &unsigned_64},
      // Count is Library's ordinary size alias.
      // TODO: Actually alias Unsigned_64 so count can have it's own
      // documentation about being the canonical size type.
      {"Count"_view, &unsigned_64},
    }};

using StandardTypes =
    Perimortem::Utility::Table<const Ttx::Model::Type*, standard_source>;

auto check_types(Perimortem::Core::View::Bytes name)
    -> Perimortem::Utility::Option<const Ttx::Model::Type&> {
  return StandardTypes::find(name).visit(
      []() -> Perimortem::Utility::Option<const Ttx::Model::Type&> {
        return {};
      },
      [](const Ttx::Model::Type* type)
          -> Perimortem::Utility::Option<const Ttx::Model::Type&> {
        return *type;
      });
}
