// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/build/product_description.hpp"
#include "tetrodotoxin/environment/plan.hpp"
#include "tetrodotoxin/language/monograph.hpp"

namespace Tetrodotoxin::Build {

// A Build Monograph retains only authored infrastructure intent. Its Package
// source remains a locator until the invocation constructs the child Workspace
// with the requested SDK providers. Keeping the request separate lets the
// selected Terminal own generated artifacts and their target policy.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);
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
