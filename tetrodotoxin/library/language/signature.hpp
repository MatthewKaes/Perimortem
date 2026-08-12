// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/layout.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

class Function;

// Signature owns exactly the authored parameter and result Layout models for
// one Function. Each model retains its source descriptors and final semantic
// entries, so Signature coordinates the two roles without copying slots,
// names, Type routes, or resolved Layouts into another representation.
class Signature {
 public:
  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Signature&>;

  Signature(const Signature&) = delete;
  Signature(Signature&&) = delete;
  auto operator=(const Signature&) -> Signature& = delete;
  auto operator=(Signature&&) -> Signature& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host) -> Bool;

  constexpr auto get_parameters() const -> const Model::Layout& {
    return parameters;
  }
  constexpr auto get_results() const -> const Model::Layout& { return results; }

  auto declares_self() const -> Bool;
  auto is_linked() const -> Bool;

 private:
  friend class Function;

  constexpr Signature(Model::Layout& parameters, Model::Layout& results)
      : parameters(parameters), results(results) {}

  auto validate_publication(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host) const -> Bool;

  Model::Layout& parameters;
  Model::Layout& results;
};

}  // namespace Tetrodotoxin::Library::Language
