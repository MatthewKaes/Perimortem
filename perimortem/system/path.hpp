// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"

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
  Path(Core::View::Bytes path);
  Path(Core::View::Bytes base_file_path, Core::View::Bytes relative_path);

  constexpr auto get_view() const -> Core::View::Bytes {
    return text.slice(0, size);
  }

  constexpr auto get_file() const -> Core::View::Bytes {
    Core::View::Bytes path = get_view();
    Count file_start = 0;
    for (Count i = 0; i < path.get_size(); i++) {
      if (path[i] == '/') {
        file_start = i + 1;
      }
    }

    return path.slice(file_start);
  }

  constexpr auto get_directory() const -> Core::View::Bytes {
    Core::View::Bytes path = get_view();
    Core::View::Bytes file = get_file();
    if (file.get_size() == path.get_size()) {
      return Core::View::Bytes();
    }

    Count directory_size = path.get_size() - file.get_size() - 1;
    return directory_size == 0 && is_rooted() ? path.slice(0, 1)
                                              : path.slice(0, directory_size);
  }

  constexpr auto get_extension() const -> Core::View::Bytes {
    Core::View::Bytes file = get_file();
    for (Count i = file.get_size(); i > 0; i--) {
      if (file[i - 1] == '.' && i - 1 > 0 && i < file.get_size()) {
        return file.slice(i - 1);
      }
    }

    return Core::View::Bytes();
  }

  constexpr auto is_rooted() const -> Bool {
    return !get_view().is_empty() && text[0] == '/';
  }

  constexpr operator Core::View::Bytes() const { return get_view(); }

  constexpr auto operator==(const Path& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }

 private:
  auto append_path(Core::View::Bytes path) -> Bool;
  auto append_segment(Core::View::Bytes segment) -> Bool;
  auto pop_segment() -> Bool;

  Core::Static::Bytes<max_size> text;
  Count size = 0;
};

}  // namespace Perimortem::System
