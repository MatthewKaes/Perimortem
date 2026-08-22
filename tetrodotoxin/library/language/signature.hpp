// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/layout.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Signature owns exactly the authored parameter and result Layout models for
// one Function. Each model retains its source descriptors and final semantic
// entries, so Signature coordinates the two roles without copying slots,
// names, Type routes, or resolved Layouts into another representation.
class Signature {
 public:
  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host)
      -> Perimortem::Core::Option<Signature&>;

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& host)
      -> Perimortem::Core::Option<Signature&>;

  auto persist(Archive::Writer& writer) const -> Bool;

  auto link_restored() -> Bool;

  Signature(const Signature&) = delete;
  Signature(Signature&&) = delete;
  auto operator=(const Signature&) -> Signature& = delete;
  auto operator=(Signature&&) -> Signature& = delete;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool;

  constexpr auto get_parameters() const -> const Model::Layout& {
    return parameters;
  }
  constexpr auto get_results() const -> const Model::Layout& { return results; }

  auto declares_self() const -> Bool;
  auto is_linked() const -> Bool;

  // Signature owns publication validation for its two exact Layouts. Function
  // invokes this semantic operation without receiving private access to the
  // Signature representation.
  auto validate_publication(Ttx::Lexical::Cursor& cursor) const -> Bool;

 private:
  constexpr Signature(
      const Ttx::Concept::Abstract& host,
      Model::Layout& parameters,
      Model::Layout& results)
      : host(host), parameters(parameters), results(results) {}

  const Ttx::Concept::Abstract& host;
  Model::Layout& parameters;
  Model::Layout& results;
};

}  // namespace Tetrodotoxin::Library::Language
