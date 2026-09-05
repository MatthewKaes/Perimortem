// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/render/language/layout.hpp"
#include "ttx/ffi/cpp/callable.hpp"

namespace Tetrodotoxin::Render::Language {

class Stage : public Ttx::Model::Callable {
 public:

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Layout& parameters,
      Layout& results) -> Stage&;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto link_restored() -> Bool;

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_parameter_layout() const -> const Layout& {
    return parameters;
  }

  constexpr auto get_result_layout() const -> const Layout& { return results; }

 private:
  constexpr Stage(
      Tetrodotoxin::Language::Definition& definition,
      Layout& parameters,
      Layout& results)
      : definition(definition), parameters(parameters), results(results) {}

  Tetrodotoxin::Language::Definition& definition;
  Layout& parameters;
  Layout& results;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Render::Language
