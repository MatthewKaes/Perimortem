// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/ffi/cpp/addressable.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Addressable owns Library receiver traversal while the TTX base exposes only
// the host-neutral Domain relationship. Plain context lookup stays closed on
// an instance because members require an explicit access operator.
class Addressable : public Ttx::Model::Addressable {
 public:

  // The owning declaration context drives these ordered barriers through the
  // category it already retained. Explicit Type routes settle before inference,
  // every Type settles before initializer linking, and all initializers link
  // before const folding. Immediate Addressables keep the neutral behavior.
  virtual auto link_declaration_type(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_inferred_declaration_type(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_declaration_initializer(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_declaration_constant(Ttx::Lexical::Cursor&) const -> Bool {
    return True;
  }

  virtual auto finalize_declaration(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_restored_declaration_type() -> Bool { return True; }

  virtual auto link_restored_declaration_initializer() -> Bool { return True; }

  virtual constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return {};
  }

  // Instance Layout assembly retains this exact Addressable identity, but the
  // declaration alone knows whether its evaluation policy creates storage.
  // Neutral Addressables remain contextual or flow identities.
  virtual constexpr auto contributes_to_instance_layout() const -> Bool {
    return False;
  }

  // Mutation is declaration authority rather than a property inferred from
  // concrete storage classes. Neutral and computed Addressables are read only.
  virtual auto permits_write_from(const Type&) const -> Bool { return False; }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    const Ttx::Concept::Abstract& resolved = resolve();
    if (&resolved != this) {
      return resolved.resolve_concept(route);
    }

    return Ttx::Concept::Unknown::get_unknown();
  }

  virtual constexpr auto get_type() const -> const Ttx::Concept::Abstract& = 0;

  constexpr auto get_domain() const -> const Ttx::Concept::Abstract& override {
    return get_type();
  }
};

}  // namespace Tetrodotoxin::Library::Language::Model
