// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/dialect/dialect.hpp"

namespace Tetrodotoxin::Syntax::Dialect {

class Entity : public Dialect {
 public:
  auto get_kind() const -> Lexical::Class override;
  auto find_attribute(Perimortem::Core::View::Bytes name) const
      -> AttributeHandler override;
};

}  // namespace Tetrodotoxin::Syntax::Dialect
