// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Type is the common Library specialization of the host neutral Type graph.
// It owns the Library operations that every concrete Library Type must answer
// without placing those operations on TTX or manufacturing an operation
// Abstract.
class Type : public Ttx::Model::Type {
 public:
  using CallableBindings = Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>;
  using Callables = Perimortem::Core::View::Selection<CallableBindings>;

  // Access makes receiver intent explicit at every Library Type query. Static
  // selects through a Type identity, while Self selects through one real
  // Addressable instance. There is no implicit overload that guesses the role.
  enum class Access : ::U8 {
    Self,
    Static,
  };

  TTX_CONTRACT(Type, Ttx::Model::Type);

  // Every completed nonempty Library Type owns one total semantic default.
  // The Arena is only the destination for the resulting Pack. Representation
  // policy and recursive construction stay with the concrete Type.
  virtual auto create_default(Perimortem::Memory::Allocator::Arena&) const
      -> Perimortem::Core::Option<Pack&> = 0;

  // Postfix propagation asks its exact receiver Type for both observable flow
  // edges. The continuation Type remains the expression result. An optional
  // error Type produces a typed Function escape, while absence produces empty
  // flow. Types that do not support propagation return no continuation Type.
  virtual constexpr auto get_propagated_type() const
      -> Perimortem::Core::Option<const Type&> {
    return {};
  }

  virtual constexpr auto get_propagated_error_type() const
      -> Perimortem::Core::Option<const Type&> {
    return {};
  }

  // Folding preserves the same split. A selected Pack continues locally,
  // absence takes the escape edge, and failure reports an incompatible Constant
  // representation to the owning Expression.
  virtual auto fold_propagation(Pack&) const
      -> Perimortem::Utility::Result<Perimortem::Core::Option<Pack&>, Bool> {
    return False;
  }

  // A value edge requires only the physical carrier closure. Declaration
  // inventories remain owned by the Type's module traversal and are not
  // imported merely because a Callable transports this Type.

  virtual auto persist(Archive::Writer& writer) const -> Bool;

  // Iteration is selected by the exact input Type. The loop supplies its real
  // binding Layout and input Pack, while each iterable Type owns admission and
  // exposes its semantic contents to a terminal producer.
  virtual auto accepts_iteration(const Ttx::Concept::Layout&) const -> Bool {
    return False;
  }

  virtual constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return {};
  }

  // An explicit initializer argument list is a receiving Type operation, not
  // an Initializer category switch. Neutral Types reject supplied flow while
  // a Type with a construction specialization owns its admission, ordering,
  // and completed value Pack. The Anchor keeps rejection on the authored
  // expression without retaining parser state in the Type.
  virtual auto create_supplied(
      Ttx::Lexical::Cursor& cursor,
      Pack&,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&>,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Pack&> {
    cursor.create_expression_error(
        anchor,
        "Selected Type does not accept supplied initializer values."_view,
        "Omit the argument list to request the selected Type's default."_view);
    return {};
  }

  virtual auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena&,
      Pack&,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&>) const
      -> Perimortem::Core::Option<Pack&> {
    return {};
  }

  // A restored Interface aggregate exposes its exact Type owner. The
  // Type supplies the provider's admitted Field inventory while the terminal
  // producer owns its target ABI and native invocation.

  // Receiving a Pack is Type policy because a target may admit flow that its
  // stored Layout cannot represent before construction. The ordinary policy
  // keeps exact Pack fitting while a concrete Type may own another accepted
  // source shape.
  virtual auto accepts(const Pack& source) const -> Bool {
    return source.fits(*this);
  }

  // A receiving Type may construct the immutable state selected by an
  // accepted Pack. Absence leaves constant folding with the source producer
  // and does not invent a generic conversion result.
  virtual auto create_fitted(Perimortem::Memory::Allocator::Arena&, Pack&) const
      -> Perimortem::Core::Option<Pack&> {
    return {};
  }

  // Authored Type closure crosses ordered barriers because later declarations
  // may query identities settled by an earlier one. The declaration context
  // drives those barriers through this protocol, while immediate and generated
  // Types keep the neutral behavior because they own no delayed graph edges.
  virtual auto link_aliases() -> Count { return 0; }

  virtual auto validate_aliases(Ttx::Lexical::Cursor&) const -> Bool {
    return True;
  }

  virtual auto link_types(Ttx::Lexical::Cursor&) -> Bool { return True; }

  virtual auto link_callable_signatures(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_fields(Ttx::Lexical::Cursor&) -> Bool { return True; }

  virtual auto validate_layout(Ttx::Lexical::Cursor&) const -> Bool {
    return True;
  }

  virtual auto link_initializers(Ttx::Lexical::Cursor&) -> Bool { return True; }

  virtual auto link_callable_bodies(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto finalize(Ttx::Lexical::Cursor&) -> Bool { return True; }

  virtual auto link_restored_types() -> Bool { return True; }

  virtual auto link_restored_callable_signatures() -> Bool { return True; }

  virtual auto link_restored_fields() -> Bool { return True; }

  virtual auto link_restored_initializers() -> Bool { return True; }

  virtual auto finalize_restored() -> Bool { return True; }

  // Visibility follows the real Type graph. A generated Type grants only its
  // own authority, while an authored contextual Type may forward through its
  // exact host without exposing that host as a second ancestry model.
  virtual auto has_private_access_to(const Type& owner) const -> Bool {
    return this == &owner;
  }

  // Declaration contexts may admit private roots before an explicit suffix
  // returns to ordinary public lookup. Types without authored declarations use
  // the ordinary context query unchanged.
  virtual auto resolve_lexical_context(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract& {
    return resolve_context(route);
  }

  // Publication proves the selected identity through the host Type rather
  // than inspecting a concrete declaration category at each consumer.
  virtual auto is_externally_reachable(const Type& type) const -> Bool {
    return &resolve_context(type.get_name()).resolve() == &type;
  }

  // The host proves caller authority only. It never supplies an implicit
  // receiver or a second lookup path. Each Type owns the exact Static and Self
  // surfaces it supports and may reject either role independently.
  virtual auto resolve_type_access(
      const Ttx::Concept::Abstract&,
      Perimortem::Core::View::Bytes,
      Access) const -> const Ttx::Concept::Abstract& {
    return Ttx::Concept::Invalid::get_invalid();
  }

  virtual auto resolve_type_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route,
      Access access) const -> const Ttx::Concept::Abstract&;

  // Lookup, reflection, and completion enumerate the same exact Callable
  // identities. Visibility selects caller access without creating another
  // generated or authored category.
  auto get_callables(
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Private) const -> Callables;

 protected:
  // Concrete Type construction publishes every authored or generated Callable
  // into this one surface. The Callable parameter Layout remains the only
  // Static or Self role authority.
  auto can_publish_callable(const Ttx::Concept::Abstract& callable) const
      -> Bool;

  auto publish_callable(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Concept::Abstract& callable,
      Bool published) -> void;

  auto get_callable_bindings(
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Private) const
      -> CallableBindings;

 private:
  Perimortem::Core::Option<Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>>
      callables;
  Perimortem::Core::Option<Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>>
      published_callables;
};

}  // namespace Tetrodotoxin::Library::Language::Model
