// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/model/package/source.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Parser::Package {

// Source consumes the root package.ttx document and resolves its declarations
// into the Package model owned by the Cursor arena. Parser state never becomes
// part of that model.
//
// This parser does not open files, search repositories, construct Environment,
// or retain a token position for another parse. A Package body is rejected
// until its Dialect can evaluate that body directly into semantic state.
class Source {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<const Model::Package::Source&>;
};

}  // namespace Tetrodotoxin::Parser::Package
