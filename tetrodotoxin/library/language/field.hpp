// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

// Field is the exact Addressable member retained by a Library Structure. Its
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

  // Source is the subordinate authored record retained until the containing
  // Structure can resolve one exact Type and construct the real Field edge.
  class Source {
   public:
    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    auto get_type_access() const
        -> Perimortem::Core::Option<const Access::Type&> {
      return type_access.visit(
          []() -> Perimortem::Core::Option<const Access::Type&> { return {}; },
          [](const Access::Type& selected)
              -> Perimortem::Core::Option<const Access::Type&> {
            return selected;
          });
    }

    constexpr auto get_documentation() const
        -> const Ttx::Concept::Documentation& {
      return documentation;
    }

    constexpr auto get_exposure() const -> Exposure { return exposure; }

    constexpr auto get_writability() const -> Writability {
      return writability;
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

    auto get_initializer() const -> Perimortem::Core::Option<const Expression&>;

    auto get_initializer() -> Perimortem::Core::Option<Expression&>;

    constexpr Source(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::Option<Access::Type> type_access,
        const Ttx::Concept::Documentation& documentation,
        Exposure exposure,
        Writability writability,
        Ttx::Lexical::Anchor anchor,
        Perimortem::Core::Option<Expression&> initializer)
        : name(name),
          type_access(type_access),
          documentation(documentation),
          exposure(exposure),
          writability(writability),
          anchor(anchor),
          initializer(initializer) {}

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::Option<Access::Type> type_access;
    const Ttx::Concept::Documentation& documentation;
    Exposure exposure;
    Writability writability;
    Ttx::Lexical::Anchor anchor;
    Perimortem::Core::Option<Expression&> initializer;
  };

 private:
  constexpr Field(
      Source& source,
      Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
          type,
      const Ttx::Model::Type& host)
      : source(source),
        type(type),
        host(host),
        initializer_linked(!source.has_initializer()) {}

 public:
  using ClassCatagory = Field;
  static constexpr Perimortem::System::Uuid contract_id{
    0xc6fc7cb2676b4bac,
    0xa205fab02169b1ca,
  };

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Source>;

  static auto link(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host,
      Source& field,
      const Ttx::Concept::Abstract& selected)
      -> Perimortem::Core::Option<Field&>;

  static auto link_inferred(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Monograph& source,
      Materializations& materializations,
      const Ttx::Model::Type& host,
      Source& field) -> Perimortem::Core::Option<Field&>;

  Field(const Field&) = delete;
  Field(Field&&) = delete;
  auto operator=(const Field&) -> Field& = delete;
  auto operator=(Field&&) -> Field& = delete;

  auto link_initializer(
      Tetrodotoxin::Language::Monograph& source,
      Materializations& materializations) -> Bool;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressable::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return source.get_name();
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return source.get_documentation();
  }

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type->get();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_type_access() const
      -> Perimortem::Core::Option<const Access::Type&> {
    return source.get_type_access();
  }

  constexpr auto get_exposure() const -> Exposure {
    return source.get_exposure();
  }

  constexpr auto get_writability() const -> Writability {
    return source.get_writability();
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return source.get_anchor();
  }

  auto get_type_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return source.get_type_anchor();
  }

  constexpr auto is_readable_externally() const -> Bool {
    return get_exposure() != Exposure::Private;
  }

  constexpr auto get_host() const -> const Ttx::Model::Type& { return host; }

  auto get_initializer() const -> Perimortem::Core::Option<const Expression&>;

  constexpr auto is_linked() const -> Bool { return initializer_linked; }

 private:
  Source& source;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
  const Ttx::Model::Type& host;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language
