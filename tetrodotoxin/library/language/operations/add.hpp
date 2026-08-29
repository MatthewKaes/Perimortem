// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Add owns one ordered scalar sum. It retains the exact left and right
// Expression edges and selects their shared Type during semantic linking.
// Folding projects a sum without changing those authored facts.
class Add : public Operation {
 public:
  BINARY_OP_CONTRACT(Add);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto select_type(const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Add(Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& left,
      Model::Pack& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
