// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/library/archive/tag.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Library::Archive {

// Reader is a bounded cursor over one Library payload or record. Its nested
// Record exposes only the frozen wire tag, optionality, and exact payload.
// Semantic reconstruction remains with the owner selected by that tag.
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

  constexpr auto is_complete() const -> Bool {
    return location == payload.get_size();
  }

 private:
  auto take(Count size)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  Perimortem::Core::View::Bytes payload;
  Count location = 0;
};

}  // namespace Tetrodotoxin::Library::Archive
