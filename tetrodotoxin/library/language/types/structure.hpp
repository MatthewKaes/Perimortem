// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure is one authored Library Type. It retains ordered Field owners and
// existing Function identities until linking can publish their real TTX edges
// through one Structured Layout.
class Structure : public Ttx::Model::Type {
 private:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Tetrodotoxin::Language::Monograph& parent,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor name_anchor);

 public:
  using ClassCatagory = Structure;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe3773c0325224200,
    0xaeb9a3131139c16f,
  };

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Monograph& parent,
      Materializations& materializations)
      -> Perimortem::Utility::Option<Structure&>;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  auto link_fields() -> Bool;
  auto link_callable_signatures() -> Bool;
  auto link_callable_bodies() -> Bool;
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

  auto get_fields() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>;

  auto get_public_fields() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>;

  auto get_callables() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Function>>;

  auto get_public_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Function>>;

  auto get_authored_fields() const -> Perimortem::Core::View::Vector<Field>;

  constexpr auto is_linked() const -> Bool {
    return stage >= Stage::FieldsLinked;
  }

  constexpr auto is_finalized() const -> Bool {
    return stage == Stage::Finalized;
  }

 private:
  enum class Stage : Unsigned_8 {
    Authored,
    FieldsLinked,
    CallableSignaturesLinked,
    CallablesLinked,
    Finalized,
  };

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Tetrodotoxin::Language::Monograph& parent;
  Ttx::Lexical::Anchor anchor;
  Ttx::Lexical::Anchor name_anchor;
  Perimortem::Memory::Managed::Vector<Field> authored_fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      public_fields;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Function>>
      callables;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Function>>
      public_callables;
  Perimortem::Utility::Option<const Ttx::Model::Layouts::Structured&> layout;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
