// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/null_terminated.hpp"

namespace Tetrodotoxin::Archiver {

// Fixed Puffer Buffer header and table directory.
//
// Each table owns one fixed-width offset immediately after the type header.
// Readers can therefore seek directly to a table without retaining a decoded
// predecessor. A format version adds directory slots when it adds tables.
class Format {
 public:
  enum class Table : Bits_8 {
    Manifest,
    References,
    Package,
    Linkages,
    End,
  };

  static constexpr auto magic = "TTXP"_view;
  static constexpr Bits_32 format_version = 10;
  static constexpr Count version_offset = magic.get_size() + sizeof(Bits_32);
  static constexpr Count header_size = version_offset + sizeof(Bits_64) * 2;
  static constexpr Count table_count = Count(Table::End);
  static constexpr Count directory_size = table_count * sizeof(Bits_64);
  static constexpr Count data_offset = header_size + directory_size;

  static constexpr auto slot(Table table) -> Count {
    return header_size + Count(table) * sizeof(Bits_64);
  }
};

}  // namespace Tetrodotoxin::Archiver
