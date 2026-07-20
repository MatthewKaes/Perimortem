// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/dialect.hpp"

namespace Tetrodotoxin::Model::Dialects {

// Package is the selectable Dialect for a package source body. It composes
// Definitions with the `public` modifier and the Alias and Group handlers.
// Successful evaluation produces a source backed Model::Package whose public
// surface contains only the authored exports. Source remains the lossless
// authoring graph and lifetime owner. Package is evaluation policy and never
// represents the resulting model.
class Package final : public Dialect {
 public:
  using ContractOwner = Package;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd776889adcaa4421,
    0xb42adf9c343268dd,
  };
  static constexpr Perimortem::Core::View::Bytes name = "Package"_view;

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> const Ttx::Concept::Abstract& override;
};

}  // namespace Tetrodotoxin::Model::Dialects
