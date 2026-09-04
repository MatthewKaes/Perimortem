// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/domain.hpp"

using namespace Ttx;

void Domain::domain(ttx_abstract self, ttx_domain_result result) const {
  result.operations->resolved(result, self, layout());
}
