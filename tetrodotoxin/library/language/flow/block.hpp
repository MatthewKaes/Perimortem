// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Block is one authored Function body or nested lexical scope. It retains exact
// statement identities in source order while the concrete statement owners
// retain their grammar and semantics. Its lexical parent, owning Function, and
// host Type remain independent facts.
// This is not a lowered basic block and owns no predecessor arguments, result
// Layout, SSA edges, or target control flow.
class Block : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(
      Block,
      Ttx::Concept::Abstract,
      0x833edbf9ef0e42de,
      0x82cdbc6fdf4e7805);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Ttx::Model::Callable& function,
      const Ttx::Model::Type& access_scope) -> Perimortem::Core::Option<Block&>;

  Block(const Block&) = delete;
  Block(Block&&) = delete;
  auto operator=(const Block&) -> Block& = delete;
  auto operator=(Block&&) -> Block& = delete;

  auto link(Tetrodotoxin::Language::Monograph& source) -> Bool;

  auto finalize() -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Block"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_statements() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>> {
    return statements;
  }

 private:
  Block(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& lexical_context,
      Ttx::Model::Callable& function,
      const Ttx::Model::Type& access_scope)
      : lexical_context(lexical_context),
        function(function),
        access_scope(access_scope),
        statements(domain) {}

  const Ttx::Concept::Abstract& lexical_context;
  Ttx::Model::Callable& function;
  const Ttx::Model::Type& access_scope;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      statements;
  Count visible_statement_count = 0;
  Ttx::Lexical::Anchor anchor = Ttx::Lexical::Anchor::create({});
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
