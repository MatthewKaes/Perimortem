// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/path.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

Path::Path(View::Bytes path) {
  if (!path.is_empty() && (path[0] == '/' || path[0] == '\\')) {
    text[size++] = '/';
  }

  Bool appended = append_path(path);
  if (!appended || size == 0) {
    size = 0;
  }
}

Path::Path(View::Bytes base_file_path, View::Bytes relative_path) {
  const Bool rooted = !relative_path.is_empty() &&
                      (relative_path[0] == '/' || relative_path[0] == '\\');
  const Bool base_rooted =
      !base_file_path.is_empty() &&
      (base_file_path[0] == '/' || base_file_path[0] == '\\');
  if (rooted || base_rooted) {
    text[size++] = '/';
  }

  if (!rooted) {
    Count directory_size = 0;
    for (Count i = 0; i < base_file_path.get_size(); i++) {
      if (base_file_path[i] == '/' || base_file_path[i] == '\\') {
        directory_size = i;
      }
    }

    Bool base_appended = append_path(base_file_path.slice(0, directory_size));
    if (!base_appended) {
      size = 0;
      return;
    }
  }

  Bool relative_appended = append_path(relative_path);
  if (!relative_appended || size == 0) {
    size = 0;
  }
}

auto Path::append_path(View::Bytes path) -> Bool {
  Count i = 0;
  while (i < path.get_size()) {
    while (i < path.get_size() && (path[i] == '/' || path[i] == '\\')) {
      i++;
    }

    const Count start = i;
    while (i < path.get_size() && path[i] != '/' && path[i] != '\\') {
      i++;
    }

    const View::Bytes segment = path.slice(start, i - start);
    if (segment.is_empty() || (segment.get_size() == 1 && segment[0] == '.')) {
      continue;
    }

    if (segment.get_size() == 2 && segment[0] == '.' && segment[1] == '.') {
      Bool popped = pop_segment();
      if (!popped) {
        return False;
      }

      continue;
    }

    Bool appended = append_segment(segment);
    if (!appended) {
      return False;
    }
  }

  return True;
}

auto Path::append_segment(View::Bytes segment) -> Bool {
  const Bool needs_separator = size != 0 && !(size == 1 && text[0] == '/');
  const Count required_size =
      segment.get_size() + (needs_separator ? Count(1) : Count(0));
  if (required_size > max_size - size) {
    return False;
  }

  if (needs_separator) {
    text[size++] = '/';
  }

  Data::copy(text.get_data() + size, segment.get_data(), segment.get_size());
  size += segment.get_size();
  return True;
}

auto Path::pop_segment() -> Bool {
  if (size == 0 || (size == 1 && text[0] == '/')) {
    return False;
  }

  Count segment_start = text[0] == '/' ? Count(1) : Count(0);
  for (Count i = segment_start; i < size; i++) {
    if (text[i] == '/') {
      segment_start = i;
    }
  }

  size = segment_start;
  return True;
}
