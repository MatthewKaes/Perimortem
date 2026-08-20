// Perimortem Engine
// Copyright © Matt Kaes

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
        Unsigned_16 tag,
        Bool optional,
        Perimortem::Core::View::Bytes payload)
        : tag(tag), optional(optional), payload(payload) {}

    constexpr auto get_tag() const -> Unsigned_16 { return tag; }

    constexpr auto is_optional() const -> Bool { return optional; }

    constexpr auto get_payload() const -> Perimortem::Core::View::Bytes {
      return payload;
    }

   private:
    Unsigned_16 tag;
    Bool optional;
    Perimortem::Core::View::Bytes payload;
  };

  static auto open(
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Reader>;

  constexpr Reader(Perimortem::Core::View::Bytes payload) : payload(payload) {}

  auto read_record() -> Perimortem::Core::Option<Record>;

  auto read_unsigned_8() -> Perimortem::Core::Option<Unsigned_8>;

  auto read_unsigned_16() -> Perimortem::Core::Option<Unsigned_16>;

  auto read_unsigned_32() -> Perimortem::Core::Option<Unsigned_32>;

  auto read_unsigned_64() -> Perimortem::Core::Option<Unsigned_64>;

  auto read_signed_64() -> Perimortem::Core::Option<Signed_64>;

  auto read_real_64() -> Perimortem::Core::Option<Real_64>;

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
