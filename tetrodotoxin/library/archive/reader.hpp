// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/library/archive/tag.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Library::Archive {

// Reader validates one Library payload and reconstructs fresh semantic
// identities in the supplied Arena. Nested readers keep every Format 1 record
// bounded so malformed content cannot consume a neighboring record.
class Reader {
 public:
  class Record {
   public:
    constexpr Record(
        U16 tag,
        Bool optional,
        Perimortem::Core::View::Bytes payload)
        : tag(tag), optional(optional), payload(payload) {}

    constexpr auto get_tag() const -> U16 { return tag; }

    constexpr auto is_optional() const -> Bool { return optional; }

    constexpr auto get_payload() const -> Perimortem::Core::View::Bytes {
      return payload;
    }

   private:
    U16 tag;
    Bool optional;
    Perimortem::Core::View::Bytes payload;
  };

  static auto open(
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Reader>;

  static auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // The embedding owner constructs its exact Composite subtype first. Library
  // then restores member identities into that object through its ordinary
  // declaration and completion contracts.
  static auto restore_declarations(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile,
      Language::Types::Composite& composite) -> Bool;

  // An embedding Archive treats this payload as opaque Library meaning. The
  // supplied context remains the real route owner for Generic arguments.
  static auto restore_type_reference(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile,
      const Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Language::TypeReference>;

  constexpr Reader(Perimortem::Core::View::Bytes payload) : payload(payload) {}

  auto read_record() -> Perimortem::Core::Option<Record>;

  auto read_u8() -> Perimortem::Core::Option<U8>;

  auto read_u16() -> Perimortem::Core::Option<U16>;

  auto read_u32() -> Perimortem::Core::Option<U32>;

  auto read_u64() -> Perimortem::Core::Option<U64>;

  auto read_s64() -> Perimortem::Core::Option<S64>;

  auto read_r64() -> Perimortem::Core::Option<R64>;

  auto read_bytes() -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  auto read_documentation(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<const Ttx::Concept::Documentation&>;

  auto read_attribute(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Attribute>;

  constexpr auto is_complete() const -> Bool {
    return location == payload.get_size();
  }

  constexpr auto get_remaining_size() const -> Count {
    return payload.get_size() - location;
  }

 private:
  auto take(Count size)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  Perimortem::Core::View::Bytes payload;
  Count location = 0;
};

}  // namespace Tetrodotoxin::Library::Archive
