// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Enumeration is one authored Library Type whose cases are named immutable
// values. It keeps source facts private until one exact integer storage Type
// and every Alias backed Constant are complete.
class Enumeration : public Ttx::Model::Type {
 private:
  struct SourceCase {
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes value;
    const Ttx::Concept::Documentation& documentation;
    Ttx::Lexical::Anchor anchor;
    Ttx::Lexical::Anchor name_anchor;
    Ttx::Lexical::Anchor value_anchor;
  };

  Enumeration(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes storage_route,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Tetrodotoxin::Language::Monograph& parent,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor name_anchor,
      Ttx::Lexical::Anchor storage_anchor);

 public:
  using ClassCatagory = Enumeration;
  static constexpr Perimortem::System::Uuid contract_id{
    0x1fa6d62be44749db,
    0xa9e71e448fbf3c53,
  };

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Monograph& parent)
      -> Perimortem::Utility::Option<Enumeration&>;

  Enumeration(const Enumeration&) = delete;
  Enumeration(Enumeration&&) = delete;
  auto operator=(const Enumeration&) -> Enumeration& = delete;
  auto operator=(Enumeration&&) -> Enumeration& = delete;

  auto link_storage() -> Bool;
  auto finalize() -> Bool;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Ttx::Model::Type::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_layout() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_name_anchor() const -> Ttx::Lexical::Anchor {
    return name_anchor;
  }

  constexpr auto get_storage_anchor() const -> Ttx::Lexical::Anchor {
    return storage_anchor;
  }

  auto get_storage_type() const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&>;

  auto get_cases() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>;

  constexpr auto get_case_count() const -> Count {
    return source_cases.get_size();
  }

  auto get_case_anchor(Count index) const
      -> Perimortem::Utility::Option<Ttx::Lexical::Anchor>;

  auto get_case_name_anchor(Count index) const
      -> Perimortem::Utility::Option<Ttx::Lexical::Anchor>;

  auto get_case_value_anchor(Count index) const
      -> Perimortem::Utility::Option<Ttx::Lexical::Anchor>;

  constexpr auto is_linked() const -> Bool {
    return stage >= Stage::StorageLinked;
  }

  constexpr auto is_finalized() const -> Bool {
    return stage == Stage::Finalized;
  }

 private:
  enum class Stage : ::Unsigned_8 {
    Authored,
    StorageLinked,
    Finalized,
  };

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes storage_route;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Tetrodotoxin::Language::Monograph& parent;
  Ttx::Lexical::Anchor anchor;
  Ttx::Lexical::Anchor name_anchor;
  Ttx::Lexical::Anchor storage_anchor;
  Perimortem::Memory::Managed::Vector<SourceCase> source_cases;
  Perimortem::Utility::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      storage_type;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>
      cases;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
