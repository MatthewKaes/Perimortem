// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/null_terminated.hpp"

namespace Tetrodotoxin::Archiver {

// Fixed Puffer Buffer header and section directory. Physical record and
// reference codes are private to this format version; they do not identify TTX
// contracts or escape into the semantic model.
class Format {
 public:
  enum class Section : Unsigned_8 {
    Manifest = 0,
    Strings = 1,
    Graph = 2,
    Terminals = 3,
    End = 4,
  };

  enum class RecordCode : Unsigned_8 {
    Namespace = 1,
    Alias = 2,
  };

  enum class ReferenceCode : Unsigned_8 {
    Local = 1,
    Dependency = 2,
    DependencyDefinition = 3,
  };

  static constexpr auto magic = "TTXP"_view;
  static constexpr Unsigned_32 format_version = 16;
  static constexpr Count section_count = Count(Section::End);
  static constexpr Count directory_count = section_count + 1;
  static constexpr Count header_size = magic.get_size() + sizeof(Unsigned_32) +
                                       directory_count * sizeof(Unsigned_64);
  static constexpr Count max_count = Count(1) << 20;

  static constexpr auto slot(Section section) -> Count {
    return magic.get_size() + sizeof(Unsigned_32) +
           Count(section) * sizeof(Unsigned_64);
  }
};

}  // namespace Tetrodotoxin::Archiver
