// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/pack.h"

const ttx_layout* ttx_pack_layout(const ttx_pack* pack) {
  return pack->operations->layout(pack);
}
