// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/library/archive/tag.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Library::Archive {

// Writer turns one completed Library Monograph into a deterministic payload.
// The public operation walks semantic identities while the record operations
// below keep Format 1 framing and scalar encoding in one place.
class Writer {
 public:
  class Record {
   public:
    constexpr auto get_offset() const -> Count { return offset; }

   private:
    friend class Writer;

    constexpr explicit Record(Count offset) : offset(offset) {}

    Count offset;
  };

  Writer(Tetrodotoxin::Language::Persistence::Profile profile);

  static auto write(
      const Language::Monograph& monograph,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  auto begin(Tag tag, Bool optional = False) -> Record;

  auto finish(Record record) -> Bool;

  auto write(U8 value) -> void;

  auto write(U16 value) -> void;

  auto write(U32 value) -> void;

  auto write(U64 value) -> void;

  auto write(S64 value) -> void;

  auto write(R64 value) -> void;

  auto write(Perimortem::Core::View::Bytes value) -> Bool;

  auto write(const Ttx::Concept::Documentation& documentation) -> Bool;

  constexpr auto get_profile() const
      -> Tetrodotoxin::Language::Persistence::Profile {
    return profile;
  }

  auto take() -> Perimortem::Memory::Dynamic::Bytes;

 private:
  Tetrodotoxin::Language::Persistence::Profile profile;
  Perimortem::Memory::Dynamic::Bytes bytes;
};

}  // namespace Tetrodotoxin::Library::Archive
