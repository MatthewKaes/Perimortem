// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/error.hpp"

#include "ttx/query.hpp"

using namespace Tetrodotoxin;

extern "C" ttx_abstract TTX_CALL tetrodotoxin_error_requirement(void) {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Language.Error"_bytes);
  return requirement.get_abi();
}

auto Language::Error::requirement() -> ttx_abstract {
  return tetrodotoxin_error_requirement();
}

auto Language::Error::recognizes(ttx_abstract candidate) -> Bool {
  const ttx_interface_relation relation =
      Ttx::relation(candidate, requirement());
  return relation == TTX_INTERFACE_SATISFIED ||
                 relation == TTX_INTERFACE_EQUIVALENT
             ? True
             : False;
}

auto Language::Error::negotiate(ttx_abstract requested) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requested, requirement())
             ? TTX_INTERFACE_SATISFIED
             : Ttx::Concept::Abstract::negotiate(requested);
}
