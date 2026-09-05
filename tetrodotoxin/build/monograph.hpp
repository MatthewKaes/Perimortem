// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/environment/dialect.hpp"
#include "tetrodotoxin/language/monograph.hpp"

namespace Tetrodotoxin::Build {

using ProductDescription = Tetrodotoxin::Environment::ProductDescription;

// A Build Monograph retains only authored infrastructure intent. Its Package
// source remains a confined locator until Environment constructs the child
// Workspace, and plugin names remain deferred artifact locators until that
// Environment admits and visits their exports. Generated files never enter
// this owner.
class Monograph final : public Tetrodotoxin::Language::Monograph {
 public:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Perimortem::Core::View::Bytes source_path,
      const Tetrodotoxin::Environment::Plan& environment,
      Perimortem::Memory::Managed::Vector<ProductDescription> products);

  TTX_NAME("Build"_view);

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool override;
  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;

  constexpr auto get_package_locator() const -> Perimortem::Core::View::Bytes {
    return package_locator;
  }
  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_environment() const
      -> const Tetrodotoxin::Environment::Plan& {
    return environment;
  }
  constexpr auto get_products() const
      -> Perimortem::Core::View::Vector<ProductDescription> {
    return products;
  }

 private:
  Perimortem::Core::View::Bytes package_locator;
  Perimortem::Core::View::Bytes source_path;
  const Tetrodotoxin::Environment::Plan& environment;
  Perimortem::Memory::Managed::Vector<ProductDescription> products;
};

}  // namespace Tetrodotoxin::Build
