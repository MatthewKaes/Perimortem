// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/reference/concept/layout.hpp"

namespace Tetrodotoxin::Library::Language {

// Diagnostics renders the names, producers, and shapes already exposed by
// Abstract, Pack, and Layout. It retains no parallel Type model; each report
// queries the exact graph identities available at the failure boundary.
class Diagnostics {
 public:
  static auto write_type(
      Ttx::Lexical::Errors::Report& report,
      const Ttx::Concept::Abstract& abstract) -> void;

  static auto write_layout(
      Ttx::Lexical::Errors::Report& report,
      const Ttx::Concept::Layout& layout) -> void;

  static auto write_pack(
      Ttx::Lexical::Errors::Report& report,
      const Model::Pack& pack) -> void;
};

}  // namespace Tetrodotoxin::Library::Language
