// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/types/defined.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Enumeration is one authored Library Type whose cases are named immutable
// values. It keeps source facts private until one exact integer storage Type
// and every Alias backed Constant are complete.
class Enumeration : public Defined {
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
      Tetrodotoxin::Language::Definition& definition,
      Access::Type storage_access);

 public:
  TTX_CONTRACT(Enumeration, Defined, 0x1fa6d62be44749db, 0xa9e71e448fbf3c53);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Enumeration&>;

  Enumeration(const Enumeration&) = delete;
  Enumeration(Enumeration&&) = delete;
  auto operator=(const Enumeration&) -> Enumeration& = delete;
  auto operator=(Enumeration&&) -> Enumeration& = delete;

  auto link_storage(Tetrodotoxin::Language::Monograph& source) -> Bool;
  auto finalize(Tetrodotoxin::Language::Monograph& source) -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_layout() const -> const Ttx::Concept::Layout& override;

  auto get_storage_type() const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto get_cases() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>;

 private:
  enum class Stage : ::Unsigned_8 {
    Authored,
    StorageLinked,
    Finalized,
  };

  Perimortem::Memory::Allocator::Arena& domain;
  Access::Type storage_access;
  Perimortem::Memory::Managed::Vector<SourceCase> source_cases;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      storage_type;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>
      cases;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
