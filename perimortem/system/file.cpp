// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include <stdio.h>
#include <x86intrin.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/time.hpp"

using namespace Perimortem::System;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;

constexpr Count max_path_size = 512;
constexpr Count max_root_size = max_path_size / 2;
alignas(__m256i) Byte root[max_root_size] = {0};
Count root_size = 0;

struct FileStatus {
  Count size_in_bytes = 0;
  // Modified time in nanoseconds.
  // The granularity actually provided by the clock is system dependant.
  Time modified_time = 0;
  Bool is_file = false;
  Bool is_directory = false;
  Bool is_valid_path = false;
};

// Populating file_status for Linux
#ifdef PERI_LINUX
#include <sys/stat.h>

FileStatus get_file_status(const char* path) {
  struct stat64 stats;

  if (stat64(path, &stats) == -1) {
    return FileStatus();
  }

  FileStatus status;
  status.size_in_bytes = Count(stats.st_size);
  status.modified_time = Time(stats.st_mtim.tv_sec, stats.st_mtim.tv_nsec);
  status.is_file = Bool(stats.st_mode & S_IFREG);
  status.is_directory = Bool(stats.st_mode & S_IFDIR);
  status.is_valid_path = true;
  return status;
}
#endif

template <Count size>
constexpr auto sanatize_path(Byte (&array)[size]) -> void {
  const auto back_dash = _mm256_set1_epi8('\\');
  const auto forward_dash = _mm256_set1_epi8('/');
  for (Count i = 0; i < size; i += sizeof(__m256i)) {
    const auto vec =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(array + i));
    const auto check = _mm256_cmpeq_epi8(vec, back_dash);
    const auto blend = _mm256_blendv_epi8(vec, forward_dash, check);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(array + i), blend);
  }
}

constexpr auto create_path(Byte (&array)[max_path_size], View::Bytes path)
    -> const SignedBits_8* {
  if (root_size + path.get_size() >= max_path_size - 1) {
    return "";
  }

  Data::copy(array, root, root_size);
  Data::copy(array + root_size, path.get_data(), path.get_size());
  sanatize_path(array);

  // Null terminator for APIs.
  array[root_size + path.get_size()] = '\0';

  return reinterpret_cast<const SignedBits_8*>(array);
}

auto File::get_root_path() -> View::Bytes {
  return View::Bytes(root, root_size);
}

auto File::set_root_path(View::Bytes path) -> Bool {
  if (path.get_size() > max_root_size - 1) {
    return False;
  }

  root_size = path.get_size();
  auto access = Core::Access::Bytes(root);
  Data::copy(access.get_data(), path.get_data(), root_size);

  sanatize_path(root);

  // Root path must be a directory
  if (root[root_size - 1] != '/') {
    root[root_size++] = '/';
  }

  return True;
}

auto File::read(View::Bytes location) -> Bool {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  auto file_status = get_file_status(path);
  if (!file_status.is_file) {
    return false;
  }

  FILE* file_handle = fopen(path, "rb");
  if (!file_handle) {
    return false;
  }

  // May need to perform a resize but we don't care about the contents.
  data.forgetful_resize(file_status.size_in_bytes);
  Count read_objects = fread(
      data.get_access().get_data(), file_status.size_in_bytes, 1, file_handle);
  fclose(file_handle);

  if (read_objects != 1) {
    data.clear();
    return false;
  }

  // Update timestamps on successful read.
  read_time = file_status.modified_time.get_stamp();
  modified_time = 0;

  return true;
}

auto File::write(Core::View::Bytes location) const -> Bool {
  // Don't write invalid files
  if (!is_valid()) {
    return false;
  }

  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  FILE* file_handle = fopen(path, "wb");
  if (!file_handle) {
    return false;
  }

  auto data_view = data.get_view();
  Count write_objects =
      fwrite(data_view.get_data(), data_view.get_size(), 1, file_handle);
  fclose(file_handle);

  return write_objects == 1;
}

// Checks the state of the file against a location.
auto File::sync_status(Core::View::Bytes location) const -> File::State {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  auto file_status = get_file_status(path);
  // If the path is empty check if the file is valid.
  // If it is then it's fresh, otherwise it's a no-op.
  if (!file_status.is_valid_path) {
    return is_valid() ? File::State::Create : File::State::Original;
  }

  // If the location references a non-file then treat it as invalid.
  if (!file_status.is_file) {
    return File::State::Invalid;
  }

  // If the file was last modified at our read time and we haven't modified the
  // file in memory then assume it's the same as the original
  if (modified_time == 0) {
    if (file_status.modified_time.get_stamp() > read_time) {
      return File::State::Stale;
    }

    if (file_status.modified_time.get_stamp() == read_time) {
      return File::State::Original;
    }

    // If we have a read time which is greater than the file then we are most
    // likely pointing to a different file than the source.
    return File::State::Conflict;
  }

  // File was modified but wasn't read from disk.
  if (read_time == 0) {
    return File::State::Conflict;
  }

  // The file must have been both modified and synced with disk at some point.
  // If it was the same file we read then we are fresh, otherwise we have a
  // possible conflict in which is the true copy.
  return file_status.modified_time == read_time ? File::State::Fresh
                                                : File::State::Conflict;
}

// Syncs a memory file with a file on disk.
// If there is a `State::Conflict` the data on disk will be overwritten by the
// data in memory only if `force` is set.
//
// Return the updated state of the File in memory.
auto File::sync(Core::View::Bytes location, Bool force) -> File::State {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  auto state = sync_status(location);

  // If we force a sync than treat conflicts as having fresh data.
  if (state == File::State::Conflict && force) {
    state = File::State::Fresh;
  }

  switch (state) {
  // No operation to perform.
  case File::State::Original:
  case File::State::Conflict:
  case File::State::Invalid:
    return state;

  // Syncing a file to disk.
  case File::State::Create:
  case File::State::Fresh: {
    // If we failed to write the data then we are still fresh.
    if (!write(location)) {
      return state;
    }

    // Sync file info without a full read.
    auto file_status = get_file_status(path);
    read_time = file_status.modified_time.get_stamp();
    modified_time = 0;
    return File::State::Original;
  }

  // Syncing a file to memory.
  case File::State::Stale: {
    // If we failed to read the data then we are still stale.
    if (!read(location)) {
      return state;
    }

    return File::State::Original;
  }
  }
}

auto File::update_contents(Core::View::Bytes content) -> void {
  data = content;
  // Always get clock time after content update in the event we need to allocate
  // and copy the buffer.
  update_modified_time();
}

auto File::update_contents(Memory::Dynamic::Bytes&& content) -> void {
  data = Data::take(content);
  update_modified_time();
}

auto File::remove(Core::View::Bytes location) -> Bool {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  return ::remove(path) == 0;
}

auto File::exists(Core::View::Bytes location) -> Bool {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  return get_file_status(path).is_file;
}

auto File::update_modified_time() -> void {
  modified_time = Time::clock().get_stamp();
}
