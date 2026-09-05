// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Language::Model {

class Type;

// Propagation belongs to the Library forms that define `?` or `!`, not to
// every Domain that can produce a value. Querying the continuation and escape
// edges together lets Option, Result, and Bool preserve their own control-flow
// meaning without teaching Type a universal sum model.
// Propagation is the operation owner reached through a Library concept route.
// Keeping it beside, rather than on, Type lets Option, Result, and Flag define
// `?` and `!` while another value Domain remains completely unaware that those
// operators exist.
class Propagation final : public Ttx::Concept::Abstract {
 public:
  constexpr Propagation(
      const Type& continuation,
      Perimortem::Core::Option<const Type&> escape = {})
      : continuation_type(continuation), escape_type(escape) {}

  TTX_NAME("propagation"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto continuation() const -> const Type& {
    return continuation_type;
  }
  constexpr auto escape() const -> Perimortem::Core::Option<const Type&> {
    return escape_type;
  }

 private:
  const Type& continuation_type;
  Perimortem::Core::Option<const Type&> escape_type;
};

}  // namespace Tetrodotoxin::Library::Language::Model
