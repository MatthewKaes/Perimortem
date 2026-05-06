// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/math/basic.hpp"

#include <stdio.h>
#include <x86intrin.h>

#ifdef PERI_LINUX
#include <sys/stat.h>

constexpr auto error_code = Count(-1);
Count get_file_size(const char* path) {
  struct stat64 stats;

  if (Count(stat64(path, &stats)) == error_code || !(stats.st_mode & S_IFREG)) {
    return error_code;
  }

  return stats.st_size;
}
#endif

using namespace Perimortem::System;
using namespace Perimortem::Core;
using namespace Perimortem::Math;

constexpr Count max_path_size = 512;
constexpr Count max_root_size = max_path_size / 2;
alignas(__m256i) Byte root[max_root_size] = {0};
Count root_size = 0;

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

File::File(Count size) : data(size), valid(true) {
  data.resize(size);

  // Zero out the page to protect against writing memory buffers to disk.
  memset(data.get_access().get_data(), 0, data.get_size());
}

File::File(View::Bytes data) : data(data), valid(true) {}

auto File::read(View::Bytes location) -> File {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  Count file_size = get_file_size(path);
  if (file_size == error_code) {
    return File();
  }

  FILE* file_handle = fopen(path, "rb");
  if (!file_handle) {
    return File();
  }

  File file(file_size);
  Count read_objects =
      fread(file.get_access().get_data(), file_size, 1, file_handle);
  fclose(file_handle);

  if (read_objects != 1) {
    return File();
  }

  return file;
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

auto File::remove(Core::View::Bytes location) -> Bool {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  return ::remove(path);
}

auto File::exists(Core::View::Bytes location) -> Bool {
  Byte path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  return get_file_size(path) != error_code;
}