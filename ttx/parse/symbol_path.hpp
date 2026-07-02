// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "ttx/parse/cursor.hpp"

namespace Ttx::Parse {

// SymbolPath is a PascalCase qualified name in the source envelope.
//
// The envelope needs only symbol segments for dialect names and import targets.
// `Library`, `Package`, and `TTX::Graphics` are enough to choose a registered
// dialect or ask resolution for a package. Full type references can have
// parameter layouts and must be resolved later by the owner that can return a
// concrete type identity.
//
// Keeping SymbolPath this small prevents the source envelope from becoming a
// partial type parser. The dialect or resolution layer that owns a richer type
// expression can parse it with the imported names already available.
//
// Layout parameters, expression parsing, and dialect specific type syntax need
// imported names and richer context. Dialects and resolution can parse those
// larger forms once the envelope has named the package or dialect involved.
class SymbolPath {
 public:
  static auto parse(Cursor& cursor) -> SymbolPath;

  // Returns the leaf segment. `TTX::Graphics` therefore names `Graphics` here,
  // while callers that need the full package path should use `get_segments()`.
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    if (segments.is_empty()) {
      return Perimortem::Core::View::Bytes();
    }

    return segments[segments.get_size() - 1];
  }
  constexpr auto get_segments() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return segments;
  }
  constexpr auto get_segment_count() const -> Count {
    return segments.get_size();
  }

  // Empty paths are parse sentinels for absent or malformed envelope symbols.
  constexpr auto is_empty() const -> Bool { return segments.is_empty(); }
  auto write(Perimortem::Memory::Dynamic::Bytes& output) const -> void;

  constexpr auto operator==(const SymbolPath& rhs) const -> Bool {
    if (segments.get_size() != rhs.segments.get_size()) {
      return False;
    }

    for (Count segment_index = 0; segment_index < segments.get_size();
         segment_index++) {
      if (segments[segment_index] != rhs.segments[segment_index]) {
        return False;
      }
    }

    return True;
  }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> segments;
};

}  // namespace Ttx::Parse
