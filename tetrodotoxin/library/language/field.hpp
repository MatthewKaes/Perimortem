// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
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

    constexpr auto get_type_route() const -> Perimortem::Core::View::Bytes {
      return type_route;
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

    constexpr auto get_type_anchor() const -> Ttx::Lexical::Anchor {
      return type_anchor;
    }

    constexpr auto is_state() const -> Bool {
      return writability == Writability::Internal;
    }

    constexpr auto is_exposed() const -> Bool {
      return exposure == Exposure::Exposed;
    }

    constexpr auto has_initializer() const -> Bool { return Bool(initializer); }

    auto get_initializer() const
        -> Perimortem::Utility::Option<const Expression&>;

    auto get_initializer() -> Perimortem::Utility::Option<Expression&>;

    constexpr Source(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Bytes type_route,
        const Ttx::Concept::Documentation& documentation,
        Exposure exposure,
        Writability writability,
        Ttx::Lexical::Anchor anchor,
        Ttx::Lexical::Anchor type_anchor,
        Perimortem::Utility::Option<Expression&> initializer)
        : name(name),
          type_route(type_route),
          documentation(documentation),
          exposure(exposure),
          writability(writability),
          anchor(anchor),
          type_anchor(type_anchor),
          initializer(initializer) {}

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes type_route;
    const Ttx::Concept::Documentation& documentation;
    Exposure exposure;
    Writability writability;
    Ttx::Lexical::Anchor anchor;
    Ttx::Lexical::Anchor type_anchor;
    Perimortem::Utility::Option<Expression&> initializer;
  };

 private:
  constexpr Field(
      Source& source,
      const Ttx::Model::Type& type,
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
      -> Perimortem::Utility::Option<Source>;

  static auto link(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host,
      Source& field,
      const Ttx::Concept::Abstract& selected)
      -> Perimortem::Utility::Option<Field&>;

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
    return type;
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type_route() const -> Perimortem::Core::View::Bytes {
    return source.get_type_route();
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

  constexpr auto get_type_anchor() const -> Ttx::Lexical::Anchor {
    return source.get_type_anchor();
  }

  constexpr auto is_state() const -> Bool { return source.is_state(); }

  constexpr auto is_exposed() const -> Bool { return source.is_exposed(); }

  constexpr auto is_readable_externally() const -> Bool {
    return get_exposure() != Exposure::Private;
  }

  constexpr auto get_host() const -> const Ttx::Model::Type& { return host; }

  auto get_initializer() const
      -> Perimortem::Utility::Option<const Expression&>;

  constexpr auto is_linked() const -> Bool { return initializer_linked; }

 private:
  Source& source;
  const Ttx::Model::Type& type;
  const Ttx::Model::Type& host;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language
