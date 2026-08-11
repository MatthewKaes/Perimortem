// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Signature owns the authored parameter and result syntax for one Function.
// Its private slots retain unresolved Type routes and exact Anchors until link
// can construct the real TTX Layouts without replacing the source
// representation.
class Signature {
 public:
  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Signature&>;

  Signature(const Signature&) = delete;
  Signature(Signature&&) = delete;
  auto operator=(const Signature&) -> Signature& = delete;
  auto operator=(Signature&&) -> Signature& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host) -> Bool;

  auto get_parameters() const -> const Ttx::Concept::Layout&;
  auto get_results() const -> const Ttx::Concept::Layout&;

  auto resolve_parameter(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_anchor() const
      -> const Perimortem::Core::Option<Ttx::Lexical::Anchor>& {
    return anchor;
  }

  constexpr auto get_parameter_size() const -> Count {
    return parameters.get_size();
  }

  constexpr auto get_result_size() const -> Count { return results.get_size(); }

  auto get_parameter_name(Count index) const -> Perimortem::Core::View::Bytes;
  auto get_result_name(Count index) const -> Perimortem::Core::View::Bytes;

  auto get_parameter_type_access(Count index) const
      -> Perimortem::Core::Option<const Access::Type&>;
  auto get_result_type_access(Count index) const
      -> Perimortem::Core::Option<const Access::Type&>;

  auto get_parameter_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;
  auto get_result_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;

  auto get_parameter_type_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;
  auto get_result_type_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;

  auto get_parameter_type(Count index) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;
  auto get_result_type(Count index) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  constexpr auto is_linked() const -> Bool { return linked; }

 private:
  class Slot {
   public:
    constexpr Slot(
        Perimortem::Core::Option<Access::Type> type_access,
        Ttx::Lexical::Anchor anchor,
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::Option<Ttx::Lexical::Anchor> name_anchor)
        : type_access(type_access),
          anchor(anchor),
          name(name),
          name_anchor(name_anchor) {}

    constexpr auto is_named() const -> Bool { return Bool(name_anchor); }

    Perimortem::Core::Option<Access::Type> type_access;
    Ttx::Lexical::Anchor anchor;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::Option<Ttx::Lexical::Anchor> name_anchor;
    Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
        type;
  };

  explicit Signature(Perimortem::Memory::Allocator::Arena& domain);

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Slot> parameters;
  Perimortem::Memory::Managed::Vector<Slot> results;
  Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> parameter_layout;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> result_layout;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
