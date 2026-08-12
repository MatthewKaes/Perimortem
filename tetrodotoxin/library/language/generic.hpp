// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Generic is the Types model's Abstract contract for a named compile time
// formula. It generates Types but is not itself a Type. The formula declares
// its complete ordered signature so Materializations can fit the linked
// argument Layout before asking it for one stable Type identity.
class Generic : public Ttx::Concept::Abstract {
 public:
  enum class Parameters : Unsigned_8 {
    Type,
    Unsigned_64,
    Signed_64,
    Bool,
  };

  // Semantic graph queries expose const references. Scalar arguments are
  // copied directly, while Type arguments retain their exact selected
  // identity even when its owner has not completed the Type's Layout yet.
  using Argument = Perimortem::Core::Static::
      Union<const Ttx::Model::Type&, ::Unsigned_64, ::Signed_64, ::Bool>;

  TTX_CONTRACT(
      Generic,
      Ttx::Concept::Abstract,
      0x8fe47e7b2c394bd7,
      0x9b3824546cc3bb50);

  virtual constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> = 0;

  // None means the supplied values do not satisfy this formula. Construction
  // is exposed so the independent Materializations transaction can invoke the
  // immutable formula after validating its complete key. Returning an
  // incomplete or redirected Type is rejection. The caller owns diagnostics.
  virtual auto create(
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> = 0;
};

}  // namespace Tetrodotoxin::Library::Language
