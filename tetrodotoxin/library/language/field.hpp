// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

// Field is the exact Addressable binding retained by a Library Composite. Its
// exposure decides readable lookup while Writability records who may mutate the
// reached value without widening the shared TTX Addressable contract.
class Field : public Ttx::Model::Addressable {
 public:
  enum class Writability : Unsigned_8 {
    Full,
    Internal,
    Init,
  };

  enum class Exposure : Unsigned_8 {
    Private,
    Public,
    Exposed,
  };

 private:
  constexpr Field(
      Tetrodotoxin::Language::Definition& definition,
      Perimortem::Core::Option<Access::Type> type_access,
      Ttx::Lexical::Anchor anchor,
      Perimortem::Core::Option<Expression&> initializer,
      const Ttx::Model::Type& host)
      : definition(definition),
        type_access(type_access),
        anchor(anchor),
        initializer(initializer),
        host(host),
        initializer_linked(!initializer) {}

 public:
  TTX_CONTRACT(
      Field,
      Ttx::Model::Addressable,
      0xc6fc7cb2676b4bac,
      0xa205fab02169b1ca);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      const Ttx::Model::Type& host) -> Perimortem::Core::Option<Field&>;

  auto link_type(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& selected) -> Bool;

  Field(const Field&) = delete;
  Field(Field&&) = delete;
  auto operator=(const Field&) -> Field& = delete;
  auto operator=(Field&&) -> Field& = delete;

  auto link_initializer(
      Tetrodotoxin::Language::Monograph& source,
      Materializations& materializations) -> Bool;

  TTX_NAME(definition.get_name());

  TTX_DOCUMENTATION(definition.get_documentation());

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type->get();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_type_access() const
      -> Perimortem::Core::Option<const Access::Type&> {
    return type_access.visit(
        []() -> Perimortem::Core::Option<const Access::Type&> { return {}; },
        [](const Access::Type& selected)
            -> Perimortem::Core::Option<const Access::Type&> {
          return selected;
        });
  }

  auto get_exposure() const -> Exposure;

  auto get_writability() const -> Writability;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto get_type_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return type_access.visit(
        []() -> Perimortem::Core::Option<Ttx::Lexical::Anchor> { return {}; },
        [](const Access::Type& selected)
            -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
          return selected.get_anchor();
        });
  }

  constexpr auto has_initializer() const -> Bool { return Bool(initializer); }

  constexpr auto is_inferred() const -> Bool { return !type_access; }

  constexpr auto is_readable_externally() const -> Bool {
    return get_exposure() != Exposure::Private;
  }

  constexpr auto get_host() const -> const Ttx::Model::Type& { return host; }

  auto get_initializer() const -> Perimortem::Core::Option<const Expression&>;

  constexpr auto is_linked() const -> Bool {
    return Bool(type) && initializer_linked;
  }

 private:
  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Core::Option<Access::Type> type_access;
  Ttx::Lexical::Anchor anchor;
  Perimortem::Core::Option<Expression&> initializer;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
  const Ttx::Model::Type& host;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language
