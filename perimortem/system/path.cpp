// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/path.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

static auto is_path_separator(Bits_8 value) -> Bool {
  return value == '/' || value == '\\';
}

static auto is_rooted_path(View::Bytes path) -> Bool {
  return !path.is_empty() && is_path_separator(path[0]);
}

static auto find_directory_size(View::Bytes file_path) -> Count {
  Count directory_size = 0;
  for (Count path_index = 0; path_index < file_path.get_size();
       path_index++) {
    if (is_path_separator(file_path[path_index])) {
      directory_size = path_index;
    }
  }

  return directory_size;
}

static auto append(
    Static::Bytes<Path::max_size>& output,
    Count& size,
    Bits_8 byte) -> Bool {
  if (size >= Path::max_size) {
    return False;
  }

  output[size] = byte;
  size++;
  return True;
}

static auto concat(
    Static::Bytes<Path::max_size>& output,
    Count& size,
    View::Bytes bytes) -> Bool {
  if (bytes.get_size() > Path::max_size - size) {
    return False;
  }

  Data::copy(output.get_data() + size, bytes.get_data(), bytes.get_size());
  size += bytes.get_size();
  return True;
}

static auto append_segment(
    Static::Bytes<Path::max_size>& output,
    Count& size,
    View::Bytes segment) -> Bool {
  View::Bytes view(output.get_data(), size);
  if (view != "/"_view && !view.is_empty() && !append(output, size, '/')) {
    return False;
  }

  return concat(output, size, segment);
}

static auto pop_segment(
    Static::Bytes<Path::max_size>& output,
    Count& size) -> Bool {
  View::Bytes view(output.get_data(), size);
  if (view.is_empty() || view == "/"_view) {
    return False;
  }

  Bool rooted = output[0] == '/';
  Count segment_start = rooted ? Count(1) : Count(0);
  for (Count i = segment_start; i < size; i++) {
    if (output[i] == '/') {
      segment_start = i;
    }
  }

  size = segment_start;
  return True;
}

static auto append_path(
    Static::Bytes<Path::max_size>& output,
    Count& size,
    View::Bytes path) -> Bool {
  Count path_index = 0;
  while (path_index < path.get_size()) {
    while (path_index < path.get_size() &&
           is_path_separator(path[path_index])) {
      path_index++;
    }

    Count segment_start = path_index;
    while (path_index < path.get_size() &&
           !is_path_separator(path[path_index])) {
      path_index++;
    }

    View::Bytes segment = path.slice(segment_start, path_index - segment_start);
    if (segment.is_empty() || segment == "."_view) {
      continue;
    }

    if (segment == ".."_view) {
      if (!pop_segment(output, size)) {
        return False;
      }
      continue;
    }

    if (!append_segment(output, size, segment)) {
      return False;
    }
  }

  return True;
}

static auto normalize_path(
    View::Bytes base_file_path,
    View::Bytes path,
    Static::Bytes<Path::max_size>& output,
    Count& size) -> Bool {
  Bool rooted = is_rooted_path(path);
  size = 0;

  if (rooted || (!base_file_path.is_empty() && is_rooted_path(base_file_path))) {
    if (!append(output, size, '/')) {
      return False;
    }
  }

  if (!rooted && !base_file_path.is_empty()) {
    View::Bytes base_directory =
        base_file_path.slice(0, find_directory_size(base_file_path));
    if (!append_path(output, size, base_directory)) {
      return False;
    }
  }

  if (!append_path(output, size, path)) {
    return False;
  }

  return size != 0;
}

Path::Path(View::Bytes path) {
  if (!normalize_path(View::Bytes(), path, text, size)) {
    size = 0;
  }
}

Path::Path(View::Bytes base_file_path, View::Bytes relative_path) {
  if (!normalize_path(base_file_path, relative_path, text, size)) {
    size = 0;
  }
}
