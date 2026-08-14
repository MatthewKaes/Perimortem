// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/writability.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Local is one Addressable declaration retained directly by its lexical Block.
// It owns its source name, declared or inferred Type, and initializer Pack
// without acquiring member Visibility or a shared Definition prefix. The
// declared Type receives the complete Pack Layout; inference is deliberately
// limited to one scalar output because a Local cannot silently materialize a
// new aggregate Type for composed flow.
class Local : public Ttx::Model::Addressable {
 public:
  TTX_CONTRACT(Local, Ttx::Model::Addressable);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Block& host) -> Perimortem::Core::Option<Local&>;

  Local(const Local&) = delete;
  Local(Local&&) = delete;
  auto operator=(const Local&) -> Local& = delete;
  auto operator=(Local&&) -> Local& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& access_scope) -> Bool;

  auto finalize() -> void;

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type->get();
  }

  constexpr auto get_writability() const -> Writability { return writability; }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto get_constant() const -> Perimortem::Core::Option<Model::Pack&>;

 private:
  enum class ConstantState : Unsigned_8 {
    Unresolved,
    Folding,
    Folded,
    Failed,
  };

  auto link_constant(Tetrodotoxin::Language::Monograph& source) const -> Bool;
  auto cache_constant() const -> Bool;

  constexpr Local(
      Perimortem::Memory::Allocator::Arena& domain,
      Block& host,
      Ttx::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      Writability writability,
      Perimortem::Core::Option<TypeReference> type_reference,
      Perimortem::Core::Option<Model::Pack&> initializer,
      Ttx::Lexical::Anchor anchor)
      : domain(domain),
        host(host),
        name_token(name_token),
        name(name),
        writability(writability),
        type_reference(type_reference),
        initializer(initializer),
        anchor(anchor),
        initializer_linked(!initializer) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Block& host;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  Writability writability;
  Perimortem::Core::Option<TypeReference> type_reference;
  Perimortem::Core::Option<Model::Pack&> initializer;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
  mutable Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>>
      constant;
  mutable ConstantState constant_state = ConstantState::Unresolved;
  Ttx::Lexical::Anchor anchor;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
