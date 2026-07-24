// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/dialect.hpp"
#include "tetrodotoxin/model/environment.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Tetrodotoxin::Model {

// Source owns one immutable source stream and every object derived from it.
// Construction copies the optional diagnostic path and source text into its
// arena before tokenization, so tokens, documentation, definitions, and nested
// graph edges cannot outlive their backing bytes. The Dialect paired with every
// rooted result preserves the instruction needed to regenerate equivalent
// source without retaining incidental whitespace. Every external name comes
// from the borrowed Environment. Environment construction is progressive and
// append only across every Source in the same transaction.
//
// A package container keeps that Environment alive and stops all
// materialization queries and all consumers backed by Sources before destroying
// member Sources.
//
// The Source itself is a durable, anonymous Abstract root. A resolver or cache
// associates an external name with it. Snippets and anonymous blobs use the
// same contract without fabricating an identity. Root definitions retain their
// real Alias, Type, Callable, or host contracts. Named roots resolve through
// one index. Multiple anonymous roots remain durable graph edges without
// inventing lookup keys. Cursor remains transient evaluation state over this
// owned data. Name resolution has exactly two authorities: rooted definitions
// owned here and bindings injected by Environment.
class Source : public Ttx::Concept::Abstract {
 public:
  // Root is one authored top level result and the Dialect that can
  // reproduce it. Keeping the relationship as one value prevents definition
  // and Dialect order from becoming independently mutable shadow state. The
  // Dialect is borrowed from the host's durable Dialects context and must
  // outlive the Source that retains it.
  class Root {
   public:
    constexpr Root(
        const Ttx::Concept::Abstract& definition,
        const Dialect& dialect)
        : definition(definition), dialect(dialect) {}

    constexpr auto get_definition() const -> const Ttx::Concept::Abstract& {
      return definition.get();
    }
    constexpr auto get_dialect() const -> const Dialect& {
      return dialect.get();
    }

   private:
    Ttx::Concept::Reference<Ttx::Concept::Abstract> definition;
    Ttx::Concept::Reference<Dialect> dialect;
  };

  using ContractOwner = Source;
  static constexpr Perimortem::System::Uuid contract_id{
    0x39a6aff4e0734177,
    0x852f26e0f1b27ec5,
  };

  Source(
      Environment& environment,
      Perimortem::Core::View::Bytes text,
      Perimortem::Core::View::Bytes path = {})
      : environment(environment),
        path(arena.proxy(path)),
        text(arena.proxy(text)),
        tokenizer(arena, this->text, this->path),
        roots(arena),
        definitions_by_name(arena) {}

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return {};
  }

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  // Evaluates one Dialect against a Cursor constructed from this Source's
  // Tokenizer and Arena, then retains the exact result it returns. Source owns
  // the complete transaction so a caller cannot pair its graph with objects
  // allocated by another source stream. Dialects only produce facts. They
  // never root an internal substitute into their caller.
  //
  // Package publication additionally requires the Dialect owner to complete and
  // seal every reachable mutable surface before definition IDs or immutable
  // consumers exist. `evaluate()` alone is not that finalization barrier.
  auto evaluate(const Dialect& dialect, Ttx::Lexical::Errors& errors)
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_arena() -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_environment() -> Environment& { return environment; }

  constexpr auto get_environment() const -> const Environment& {
    return environment;
  }

  constexpr auto get_tokenizer() const -> const Ttx::Lexical::Tokenizer& {
    return tokenizer;
  }

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }

  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return text;
  }

  constexpr auto get_roots() const -> Perimortem::Core::View::Vector<Root> {
    return roots;
  }

 private:
  // Publication is the commit point of Source evaluation. Keeping it private
  // prevents callers from injecting an arbitrary Abstract or inventing a
  // Dialect association outside the Cursor transaction owned by the Source.
  auto add_root(
      const Ttx::Concept::Abstract& definition,
      const Dialect& dialect) -> Bool;

  using Definitions = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>;

  Environment& environment;
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes text;
  Ttx::Lexical::Tokenizer tokenizer;
  Perimortem::Memory::Managed::Vector<Root> roots;
  Definitions definitions_by_name;
};

}  // namespace Tetrodotoxin::Model
