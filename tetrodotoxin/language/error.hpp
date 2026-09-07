// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/error.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/requirement.hpp"

namespace Tetrodotoxin::Language {

// Error lets a language report a completed failure as the identity owned by
// the subsystem that understood the request. The consumer still constructs a
// Report because only it knows which authored source and range led to the
// request, preserving acquisition, compilation, and provider failures as
// outcomes owned by those systems instead of flattening them into None or
// an enum shared by the whole process.
class Error : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("Error"_view);

  static auto requirement() -> ttx_abstract;
  static auto recognizes(ttx_abstract candidate) -> Bool;

  virtual auto describe(Ttx::Lexical::Errors::Report& report) const -> void = 0;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;
};

}  // namespace Tetrodotoxin::Language
