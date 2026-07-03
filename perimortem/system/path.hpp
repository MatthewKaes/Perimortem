// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System {

// A lexical path value with stable slash-normalized storage.
//
// Path deliberately does not touch the filesystem. It only owns the cheap path
// work that every system layer needs: separator normalization, `.` removal,
// `..` collapse, and relative resolution from a file path. Callers that care
// about mounts, roots, source trees, archives, or OS canonicalization apply
// those policies after Path has produced a compact normalized value.
class Path {
 public:
  static constexpr Count max_size = 510;

  Path() = default;
  Path(Core::View::Bytes path) { set(path); }

  auto set(Core::View::Bytes path) -> Bool;
  auto set_relative(
      Core::View::Bytes base_file_path,
      Core::View::Bytes relative_path) -> Bool;
  auto clear() -> void { text.clear(); }

  constexpr auto get_view() const -> Core::View::Bytes {
    return text.get_view();
  }
  constexpr auto is_empty() const -> Bool { return text.get_view().is_empty(); }
  constexpr auto is_rooted() const -> Bool {
    return !text.get_view().is_empty() && text.get_view()[0] == '/';
  }

  constexpr operator Core::View::Bytes() const { return get_view(); }
  constexpr auto operator==(const Path& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }

 private:
  Memory::Dynamic::Bytes text;
};

}  // namespace Perimortem::System
