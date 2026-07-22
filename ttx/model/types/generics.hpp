// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/model/type.hpp"

namespace Ttx::Model::Types {

// Generic is the Types model's Abstract contract for a named compile-time
// formula. It generates Types but is not itself a Type. The formula declares
// its complete ordered signature so the parser can validate arguments and own
// diagnostics before asking it for one stable materialized Type identity.
class Generic : public Concept::Abstract {
 public:
  enum class Parameters : Unsigned_8 {
    Type,
    Unsigned_64,
    Signed_64,
    Bool,
  };

  // Semantic graph queries expose const references. Scalar arguments are
  // copied directly, while Type arguments retain their resolved identity.
  using Argument = Perimortem::Core::Static::
      Union<const Ttx::Model::Type&, ::Unsigned_64, ::Signed_64, ::Bool>;

  using ContractOwner = Generic;
  static constexpr Perimortem::System::Uuid contract_id{
    0x8fe47e7b2c394bd7,
    0x9b3824546cc3bb50,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Concept::Abstract::implements(requested);
  }

  virtual constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> = 0;

  // None means the supplied values do not satisfy this formula. The parser
  // retains the source tokens and therefore owns the resulting diagnostic.
  // A successful formula may retain Type alternatives in its cache, so the
  // semantic graph owner must keep those referenced Types alive as long as the
  // returned materialization remains queryable.
  virtual auto find(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Perimortem::Utility::Option<Ttx::Model::Type&> = 0;
};

}  // namespace Ttx::Model::Types
