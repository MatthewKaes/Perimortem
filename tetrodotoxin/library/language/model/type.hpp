// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/visibility.hpp"
#include "tetrodotoxin/library/language/types/instance.hpp"
#include "tetrodotoxin/library/language/types/static.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/ffi/cpp/domain.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Library declarations need a common place to publish their static and
// receiver-owned names. Type adds those two authorities to a host-neutral
// Domain, allowing call syntax and source lookup to share one vocabulary.
// Construction, admission, propagation, and iteration remain sibling concepts
// supplied only by the concrete Library forms that define them.
class Type : public Ttx::Model::Domain {
 public:
  using CallableBindings =
      Perimortem::Core::View::Vector<Ttx::Concept::Abstract*>;
  using Callables = Perimortem::Core::View::Selection<CallableBindings>;


  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  virtual constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return {};
  }

  // Declaration contexts may admit private roots before an explicit suffix
  // returns to ordinary public lookup. Types without authored declarations use
  // the ordinary context query unchanged.
  virtual auto resolve_lexical_context(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract& {
    return resolve_concept(route);
  }

  // Lookup and lowering enumerate the same Callable identities that were
  // installed into Static and Self receiver authorities. Public traversal is
  // explicit so publication policy does not become a mode on Type itself.
  auto get_callables() const -> Callables;
  auto get_published_callables() const -> Callables;

 protected:
  constexpr Type() : visibility(*this) {}

  explicit Type(Perimortem::Memory::Allocator::Arena& domain);

  auto edit_static_authority() -> Types::Static&;
  auto edit_instance_authority() -> Types::Instance&;
  auto get_static_authority() const -> const Types::Static&;
  auto get_instance_authority() const -> const Types::Instance&;

  // Concrete Type construction publishes every authored or generated Callable
  // into this one surface. The Callable parameter Layout remains the only
  // Static or Self role authority.
  auto can_publish_callable(const Ttx::Concept::Abstract& callable) const
      -> Bool;

  auto publish_callable(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Concept::Abstract& callable,
      Bool published) -> void;

  auto get_callable_bindings() const -> CallableBindings;
  auto get_published_callable_bindings() const -> CallableBindings;

 private:
  auto initialize_authorities(Perimortem::Memory::Allocator::Arena& domain)
      -> void;

  Perimortem::Core::Option<Types::Static*> static_authority;
  Perimortem::Core::Option<Types::Instance*> instance_authority;
  Perimortem::Core::Option<
      Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*>>
      callables;
  Perimortem::Core::Option<
      Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*>>
      published_callables;
  ExactVisibility visibility;
};

}  // namespace Tetrodotoxin::Library::Language::Model
