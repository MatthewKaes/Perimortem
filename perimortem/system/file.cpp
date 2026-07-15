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

auto File::create_path(Bits_8* output, View::Bytes path) -> const Signed_8* {
  if (path.get_size() >= max_path_size) {
    return "";
  }

  for (Count i = 0; i < path.get_size(); i++) {
    output[i] = path[i] == '\\' ? '/' : path[i];
  }

  output[path.get_size()] = '\0';
  return Data::cast<const Signed_8>(output);
}

auto File::read(View::Bytes location) -> Dynamic::Bytes {
  Bits_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

#ifdef PERI_LINUX
  struct stat64 status;
  auto status_read = stat64(path, &status);
  if (status_read == -1 || !(status.st_mode & S_IFREG)) {
    return Dynamic::Bytes();
  }

  FILE* file = fopen(path, "rb");
  if (!file) {
    return Dynamic::Bytes();
  }

  Dynamic::Bytes data;
  data.forgetful_resize(Count(status.st_size));
  if (status.st_size != 0) {
    Count items_read =
        fread(data.get_access().get_data(), Count(status.st_size), 1, file);
    if (items_read != 1) {
      fclose(file);
      return Dynamic::Bytes();
    }
  }

  fclose(file);
  return data;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::write(View::Bytes data, View::Bytes location) -> Bool {
  Bits_8 path_buffer[max_path_size];
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
  Bits_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);
  int removed = ::remove(path);
  return removed == 0;
}

auto File::exists(View::Bytes location) -> Bool {
  Bits_8 path_buffer[max_path_size];
  const auto path = create_path(path_buffer, location);

#ifdef PERI_LINUX
  struct stat64 status;
  int status_read = stat64(path, &status);
  return status_read == 0 && Bool(status.st_mode & S_IFREG);
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}
