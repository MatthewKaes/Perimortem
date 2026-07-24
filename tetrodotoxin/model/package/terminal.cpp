// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/package/terminal.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Model;

Package::Terminal::Terminal(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes path,
    View::Bytes content)
    : path(arena.proxy(path)), content(arena.proxy(content)) {}

auto Package::Terminal::get_path() const -> View::Bytes {
  return path;
}

auto Package::Terminal::get_content() const -> View::Bytes {
  return content;
}

auto Package::Terminal::is_valid_path(View::Bytes path) -> Bool {
  if (path.is_empty() || path[0] == '/' || path[0] == '\\') {
    return False;
  }

  Count segment_start = 0;
  for (Count i = 0; i <= path.get_size(); i++) {
    if (i < path.get_size() && path[i] == '\\') {
      return False;
    }
    if (i < path.get_size() && path[i] != '/') {
      continue;
    }

    View::Bytes segment = path.slice(segment_start, i - segment_start);
    if (segment.is_empty() || (segment.get_size() == 1 && segment[0] == '.') ||
        (segment.get_size() == 2 && segment[0] == '.' && segment[1] == '.')) {
      return False;
    }

    segment_start = i + 1;
  }

  return True;
}
