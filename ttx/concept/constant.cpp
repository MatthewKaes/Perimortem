// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/constant.hpp"

using namespace Ttx;

auto Constant::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, ttx_constant_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Abstract::negotiate(requirement);
}
