// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include <stdio.h>
#ifdef PERI_LINUX
#include <sys/stat.h>
#endif

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;

// Bibliotheca can represent byte allocations through the 32 GiB archive.
// Reject the next radix before it can index beyond that owned range.
static constexpr Count max_read_size = Count(1) << 35;

// Max path size for now. Used for storing root operations across threads
// without needing to handshake dynamic memory allocations.
static constexpr Count max_path_size = 512;

static auto read_opened_file(FILE* file) -> Option<Dynamic::Bytes> {
  struct stat64 status;
  int status_read = fstat64(fileno(file), &status);
  if (status_read != 0 || !S_ISREG(status.st_mode) || status.st_size < 0) {
    return {};
  }

  Unsigned_64 size = Unsigned_64(status.st_size);
  if (size > max_read_size) {
    return {};
  }

  if (size == 0) {
    return Dynamic::Bytes();
  }

  Dynamic::Bytes data;
  data.forgetful_resize(Count(size));

  CppSize items_read =
      fread(data.get_access().get_data(), 1, CppSize(size), file);
  if (items_read != CppSize(size) || ferror(file) != 0) {
    return {};
  }

  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(data));
}

// Creates a OS specific path that is also null terminated so it can be passed
// to C ABIs.
//
// `char` is used here because C/C++ treat `char` as a special type so we can't
// use `Signed_8` or `Unsigned_8` here. :(
static auto create_path(Unsigned_8* output, View::Bytes path) -> const char* {
  if (path.get_size() >= max_path_size) {
    return "";
  }

  for (Count i = 0; i < path.get_size(); i++) {
    output[i] = path[i] == '\\' ? '/' : path[i];
  }

  output[path.get_size()] = '\0';
  return Data::cast<const char>(output);
}

auto File::read(View::Bytes location) -> Option<Dynamic::Bytes> {
  Unsigned_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

#ifdef PERI_LINUX
  FILE* file = fopen(path, "rb");
  if (!file) {
    return {};
  }

  Option<Dynamic::Bytes> data = read_opened_file(file);
  int close_result = fclose(file);
  if (close_result != 0) {
    return {};
  }

  return data;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::write(View::Bytes data, View::Bytes location) -> Bool {
  Unsigned_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

  FILE* file = fopen(path, "wb");
  if (!file) {
    return False;
  }

  Bool written = True;
  if (!data.is_empty()) {
    Count items_written = fwrite(data.get_data(), data.get_size(), 1, file);
    written = items_written == 1;
  }

  fclose(file);
  return written;
}

auto File::remove(View::Bytes location) -> Bool {
  Unsigned_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);
  int removed = ::remove(path);
  return removed == 0;
}

auto File::exists(View::Bytes location) -> Bool {
  Unsigned_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

#ifdef PERI_LINUX
  struct stat64 status;
  int status_read = stat64(path, &status);
  return status_read == 0 && Bool(status.st_mode & S_IFREG);
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}
