// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Addressable owns Library receiver traversal while the TTX base retains only
// the host neutral named edge to an exact Type. Plain context lookup stays
// closed on an instance because members require an explicit access operator.
class Addressable : public Ttx::Model::Addressable {
 public:
  TTX_CONTRACT(Addressable, Ttx::Model::Addressable);

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

  virtual auto persist(Archive::Writer& writer) const -> Bool;

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

  // Receiver shape belongs to the selected Addressable. Contextual and
  // imported identities use Static lookup by default, while declaration owners
  // may refine the exact Static or Self surfaces they provide.
  virtual constexpr auto supports_access(Type::Access access) const -> Bool {
    return access == Type::Access::Static;
  }

  // Mutation is declaration authority rather than a property inferred from
  // concrete storage classes. Neutral and computed Addressables are read only.
  virtual auto permits_write_from(const Type&) const -> Bool { return False; }

  // Constant flow is owned by the declaration that can prove it. Expressions
  // ask the selected Addressable instead of enumerating Field and Local, while
  // runtime storage and foreign bindings retain the neutral absence.
  virtual auto get_constant() const -> Perimortem::Core::Option<Pack&> {
    return {};
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    const Ttx::Concept::Abstract& resolved = resolve();
    if (&resolved != this) {
      return resolved.resolve_context(route);
    }

    return Ttx::Concept::Invalid::get_invalid();
  }

  auto resolve_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    const Ttx::Concept::Abstract& resolved = resolve();
    if (&resolved != this) {
      return resolved.resolve_access(host, route);
    }

    auto type = get_type().resolve().select<Type>();
    return type ? type->resolve_type_access(host, route, Type::Access::Self)
                : Ttx::Concept::Invalid::get_invalid();
  }

  auto resolve_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    const Ttx::Concept::Abstract& resolved = resolve();
    if (&resolved != this) {
      return resolved.resolve_call(host, route);
    }

    auto type = get_type().resolve().select<Type>();
    return type ? type->resolve_type_call(host, route, Type::Access::Self)
                : Ttx::Concept::Invalid::get_invalid();
  }

  virtual constexpr auto get_type() const -> const Type& override = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Model
