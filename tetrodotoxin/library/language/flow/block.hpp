// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Block is one authored Function body or nested lexical scope. It retains
// identity-free Statement memberships in source order while every entry keeps
// its exact graph object and source-backed Documentation. Its lexical parent,
// owning Function, and host Type remain independent facts.
// This is not a lowered basic block and owns no predecessor arguments, result
// Layout, SSA edges, or target control flow.
class Block : public Scope {
 public:
  TTX_CONTRACT(Block, Scope);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope,
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
          enclosing_loop = {}) -> Perimortem::Core::Option<Block&>;

  Block(const Block&) = delete;
  Block(Block&&) = delete;
  auto operator=(const Block&) -> Block& = delete;
  auto operator=(Block&&) -> Block& = delete;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  auto lower(Llvm::Builder& body) const -> Bool;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Block"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_statements() const
      -> Perimortem::Core::View::Vector<Statement> {
    return statements;
  }

  constexpr auto get_enclosing_loop() const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override {
    return enclosing_loop.visit(
        []() -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> {
          return {};
        },
        [](const Ttx::Concept::Reference<const Ttx::Concept::Abstract>& loop)
            -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> {
          return loop.get();
        });
  }

  constexpr auto get_function_results() const
      -> const Ttx::Concept::Layout& override {
    return function.get_results();
  }

  constexpr auto get_access_scope() const -> const Model::Type& override {
    return access_scope;
  }

 private:
  Block(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope,
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>> enclosing_loop)
      : lexical_context(lexical_context),
        function(function),
        access_scope(access_scope),
        statements(domain),
        enclosing_loop(enclosing_loop) {}

  const Ttx::Concept::Abstract& lexical_context;
  Model::Callable& function;
  const Model::Type& access_scope;
  Perimortem::Memory::Managed::Vector<Statement> statements;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      enclosing_loop;
  // Linking advances this prefix before each Statement so name lookup observes
  // only declarations whose source position precedes the active entry. It is
  // transient phase state, not another declaration inventory.
  Count linked_prefix_size = 0;
  Ttx::Lexical::Anchor anchor = Ttx::Lexical::Anchor::create({});
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
