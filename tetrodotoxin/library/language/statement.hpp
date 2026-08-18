// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Statement is one retained membership in source order, not another semantic
// identity. Its root is the complete outermost Pack returned for an expression
// Statement, or the exact declaration, control owner, or nested Block selected
// by grammar. That borrowed object remains the only Abstract in the graph. A
// compact operation table is fixed when grammar selects the root, avoiding both
// multiple inheritance and later concrete category inspection by Block.
//
// Leading Documentation belongs here because it describes participation in an
// executable sequence. The underlying semantic owner need not counterfeit a
// declaration merely to retain that presentation fact backed by source.
class Statement {
 private:
  class Continue {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const -> Bool {
      return True;
    }
  };

  class NoBindingName {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return {};
    }
  };

  class NoBinding {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> {
      return {};
    }
  };

 public:
  template <
      typename Owner,
      typename Link,
      typename Finalize,
      typename ReachesNext = Continue,
      typename BindingName = NoBindingName,
      typename Binding = NoBinding>
  static constexpr auto create(
      Owner& root,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Lexical::Anchor anchor,
      Link,
      Finalize,
      ReachesNext = {},
      BindingName = {},
      Binding = {}) -> Statement {
    static_assert(__is_base_of(Ttx::Concept::Abstract, Owner));
    return Statement(
        root, documentation, anchor,
        Operations{
          .link = [](Ttx::Concept::Abstract& root, Ttx::Lexical::Cursor& cursor,
                     Flow::Scope& scope) -> Bool {
            return Link{}(static_cast<Owner&>(root), cursor, scope);
          },
          .finalize = [](Ttx::Concept::Abstract& root,
                         Ttx::Lexical::Cursor& cursor) -> void {
            Finalize{}(static_cast<Owner&>(root), cursor);
          },
          .lower = [](const Ttx::Concept::Abstract& root,
                      Llvm::Builder& body) -> Bool {
            return static_cast<const Owner&>(root).lower(body);
          },
          .reaches_next = [](const Ttx::Concept::Abstract& root) -> Bool {
            return ReachesNext{}(static_cast<const Owner&>(root));
          },
          .get_binding_name = [](const Ttx::Concept::Abstract& root)
              -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
            return BindingName{}(static_cast<const Owner&>(root));
          },
          .get_binding = [](const Ttx::Concept::Abstract& root)
              -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> {
            return Binding{}(static_cast<const Owner&>(root));
          },
        });
  }

  constexpr auto link(Ttx::Lexical::Cursor& cursor, Flow::Scope& scope)
      -> Bool {
    return operations.link(root.get(), cursor, scope);
  }

  constexpr auto finalize(Ttx::Lexical::Cursor& cursor) -> void {
    operations.finalize(root.get(), cursor);
  }

  auto lower(Llvm::Builder& body) const -> Bool;

  constexpr auto reaches_next() const -> Bool {
    return operations.reaches_next(root.get());
  }

  constexpr auto get_binding_name() const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return operations.get_binding_name(root.get());
  }

  constexpr auto get_binding() const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> {
    return operations.get_binding(root.get());
  }

  constexpr auto get_root() const -> const Ttx::Concept::Abstract& {
    return root.get();
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  struct Operations {
    Bool (*link)(Ttx::Concept::Abstract&, Ttx::Lexical::Cursor&, Flow::Scope&);
    void (*finalize)(Ttx::Concept::Abstract&, Ttx::Lexical::Cursor&);
    Bool (*lower)(const Ttx::Concept::Abstract&, Llvm::Builder&);
    Bool (*reaches_next)(const Ttx::Concept::Abstract&);
    Perimortem::Core::Option<Perimortem::Core::View::Bytes> (*get_binding_name)(
        const Ttx::Concept::Abstract&);
    Perimortem::Core::Option<const Ttx::Concept::Abstract&> (*get_binding)(
        const Ttx::Concept::Abstract&);
  };

  constexpr Statement(
      Ttx::Concept::Abstract& root,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Lexical::Anchor anchor,
      Operations operations)
      : root(root),
        documentation(documentation),
        anchor(anchor),
        operations(operations) {}

  Ttx::Concept::Reference<Ttx::Concept::Abstract> root;
  const Ttx::Concept::Documentation& documentation;
  Ttx::Lexical::Anchor anchor;
  Operations operations;
};

// Managed::Vector grows by relocating its entries as bytes. Keeping this
// assertion beside the record prevents a later ownership field from silently
// invalidating that storage contract.
static_assert(__is_trivially_copyable(Statement));

}  // namespace Tetrodotoxin::Library::Language
