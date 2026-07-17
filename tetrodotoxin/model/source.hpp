// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/group.hpp"

namespace Tetrodotoxin::Model {

// Source is the durable root produced by evaluating one source unit. It owns
// its named definition Group directly so the source boundary is explicit
// without pretending that a module is a runtime Type or giving it an empty
// Layout.
//
// The Group contains the real Abstracts produced by the selected Dialects.
// Their typed edges are the source graph: Alias retains its target, Structured
// Layout retains Addressables, Callable retains an address, and future
// executable or shader contracts retain the facts they define. Source does not
// collect those relationships into an untyped linkage side table.
//
// Source owns no cursor, source bytes, Dialect, package version, archive index,
// terminal artifact, or diagnostic state. Those belong to the evaluator,
// resolver, Compiler, or Archiver boundary that owns this stable object.
// The contract remains derivable when a source-unit concern earns additional
// operations. A Dialect name or package role alone does not justify a subtype.
class Source : public Ttx::Concept::Abstract {
 public:
  using ContractOwner = Source;
  static constexpr Perimortem::System::Uuid contract_id{
    0x39a6aff4e0734177,
    0x852f26e0f1b27ec5,
  };

  Source(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> definitions,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Comment::get_empty());

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_definitions() const -> const Ttx::Model::Group&;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  Ttx::Model::Group definitions;
};

}  // namespace Tetrodotoxin::Model
