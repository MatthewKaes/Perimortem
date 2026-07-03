// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/path.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
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

static auto append_path_segment(
    Dynamic::Bytes& output,
    View::Bytes segment) -> Bool {
  if (output.get_view() != "/"_view && !output.get_view().is_empty()) {
    output.append('/');
  }

  output.concat(segment);
  return output.get_size() <= Path::max_size;
}

static auto pop_path_segment(Dynamic::Bytes& output) -> Bool {
  if (output.get_view().is_empty() || output.get_view() == "/"_view) {
    return False;
  }

  Bool rooted = output[0] == '/';
  Count segment_start = rooted ? Count(1) : Count(0);
  for (Count path_index = segment_start; path_index < output.get_size();
       path_index++) {
    if (output[path_index] == '/') {
      segment_start = path_index;
    }
  }

  output.resize(segment_start);
  return True;
}

static auto append_path(
    Dynamic::Bytes& output,
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
      if (!pop_path_segment(output)) {
        return False;
      }
      continue;
    }

    if (!append_path_segment(output, segment)) {
      return False;
    }
  }

  return True;
}

static auto normalize_path(
    View::Bytes base_file_path,
    View::Bytes path,
    Dynamic::Bytes& output) -> Bool {
  Bool rooted = is_rooted_path(path);
  output.clear();

  if (rooted || (!base_file_path.is_empty() && is_rooted_path(base_file_path))) {
    output.append('/');
  }

  if (!rooted && !base_file_path.is_empty()) {
    View::Bytes base_directory =
        base_file_path.slice(0, find_directory_size(base_file_path));
    if (!append_path(output, base_directory)) {
      return False;
    }
  }

  if (!append_path(output, path)) {
    return False;
  }

  return !output.get_view().is_empty();
}

auto Path::set(View::Bytes path) -> Bool {
  Dynamic::Bytes normalized;
  if (!normalize_path(View::Bytes(), path, normalized)) {
    text.clear();
    return False;
  }

  text = Data::take(normalized);
  return True;
}

auto Path::set_relative(View::Bytes base_file_path, View::Bytes relative_path)
    -> Bool {
  Dynamic::Bytes normalized;
  if (!normalize_path(base_file_path, relative_path, normalized)) {
    text.clear();
    return False;
  }

  text = Data::take(normalized);
  return True;
}
