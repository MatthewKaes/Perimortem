// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/callables/self.hpp"
#include "ttx/model/callables/static.hpp"
#include "ttx/model/exports.hpp"

namespace Tetrodotoxin::Ttx::Model::Environment {

// Namespace is a durable named resolution context produced by Tetrodotoxin
// source grammar. It exists here rather than in the TTX model because the
// abstract machine does not require namespaces. Package `group` syntax, source
// roots, and host Dialect sets are concrete Tetrodotoxin domains that earn the
// concept.
//
// A Namespace retains definitions while they are assembled and separately
// indexes the edges deliberately published for dependency lookup, reflection,
// documentation, and archive traversal. Only the published index participates
// in resolve_context(). Package groups root and export the same definitions.
// Dialects with private definitions use a distinct retained owner and public
// Namespace rather than leaking their internal context through Exports.
// Lexical parents and Dependencies are passed during interpretation and never
// become roots or exports here.
//
// Static callables remain ordinary Namespace roots and exports while also
// participating in the invocation-specific lookup surface selected by arrow
// syntax. A Namespace cannot own Self callables because it has no runtime
// receiver.
//
// Namespace is not a Type, Layout, Pack, or Scope. A query may walk through it
// and eventually prove one of those narrower contracts on the selected
// definition. Source is a separate anonymous lifetime root that may contain
// Namespaces among its definitions.
class Namespace : public Ttx::Model::Exports {
 public:
  using ContractOwner = Namespace;
  static constexpr Perimortem::System::Uuid contract_id{
    0x95d5be34987843bc,
    0x9fa23a91d291f889,
  };

  Namespace(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Documentation::get_empty());

  // Constructs one complete Namespace from an existing export view.
  static auto construct(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> exports,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Documentation::get_empty())
      -> const Ttx::Concept::Abstract&;

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_export_count() const -> Count override;
  auto get_export(Count index) const -> const Ttx::Concept::Abstract& override;

  // Retained roots are the complete semantic contents of this concrete
  // Namespace. Package canonicalization enumerates them so private definitions
  // receive stable IDs. resolve_context() remains closed over exports.
  auto get_root_count() const -> Count;
  auto get_root(Count index) const -> const Ttx::Concept::Abstract&;

  // Retains one produced definition for the current owner transaction. Rooting
  // alone never makes that name visible through the Exports context.
  auto add_root(
      const Ttx::Concept::Abstract& definition,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  // Publishes one edge in authored order. An already rooted name must identify
  // the same edge. Construction from an existing public graph may publish and
  // root the edge in one operation.
  auto add_export(
      const Ttx::Concept::Abstract& definition,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  // Exposed state remains writable in retained lookup while Exports returns
  // only the stable read-only projection supplied by that real owner.
  auto add_exposed(
      const Ttx::Model::Addressables::Writable& definition,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto add_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto resolve_root(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_exported_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  // Sealing is the Namespace-owned immutable-consumer barrier. It is a
  // one-time construction transition and every later append is rejected.
  auto seal() -> Bool;

 private:
  using RootIndex = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>;

  auto outer_contains(
      const Ttx::Concept::Abstract& outer_context,
      Perimortem::Core::View::Bytes name) const -> Bool;

  Perimortem::Core::View::Bytes name;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      roots;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      exports;
  const Ttx::Concept::Documentation& documentation;
  RootIndex roots_by_name;
  RootIndex exports_by_name;
  RootIndex statics_by_name;
  RootIndex exported_statics_by_name;
  Bool sealed = False;
};

}  // namespace Tetrodotoxin::Ttx::Model::Environment
