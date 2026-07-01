// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/parse/cursor.hpp"

namespace Ttx::Parse {

class TypePath {
 public:
  static auto parse(Cursor& cursor) -> TypePath;

  auto get_name() const -> Perimortem::Core::View::Bytes;
  auto get_segments() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>;
  auto get_segment_count() const -> Count;
  auto is_empty() const -> Bool;

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> segments;
};

}  // namespace Ttx::Parse
