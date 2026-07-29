// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef PERI_LINUX
#include <linux/openat2.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/path.hpp"

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
static constexpr Count max_warning_size = max_path_size + 256;

static auto log_file_warning(
    View::Bytes operation,
    View::Bytes path,
    View::Bytes stage,
    View::Bytes detail_name,
    Signed_64 detail) -> void {
  Count logged_path_size = path.get_size();
  if (logged_path_size != 0 && path[logged_path_size - 1] == '\0') {
    logged_path_size--;
  }

  if (logged_path_size > max_path_size) {
    logged_path_size = max_path_size;
  }

  View::Bytes logged_path(path.get_data(), logged_path_size);
  Diagnostics::Log::Message<max_warning_size> warning(
      Diagnostics::Log::Level::Warning, Diagnostics::Source());
  warning << operation << " failed. path="_view << logged_path << " stage="_view
          << stage << ' ' << detail_name << '=' << detail;
}

static auto read_opened_file(
    FILE* file,
    View::Bytes operation,
    View::Bytes path) -> Option<Dynamic::Bytes> {
  struct stat64 status;
  Signed_32 status_read = fstat64(fileno(file), &status);
  if (status_read != 0) {
    Signed_32 status_error = errno;
    log_file_warning(
        operation, path, "metadata"_view, "errno"_view, status_error);
    return {};
  }

  if (!S_ISREG(status.st_mode)) {
    log_file_warning(
        operation, path, "classification"_view, "mode"_view,
        Signed_64(status.st_mode));
    return {};
  }

  if (status.st_size < 0) {
    log_file_warning(
        operation, path, "size"_view, "value"_view, Signed_64(status.st_size));
    return {};
  }

  Unsigned_64 size = Unsigned_64(status.st_size);
  if (size > max_read_size) {
    log_file_warning(
        operation, path, "size"_view, "value"_view, Signed_64(size));
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
    Signed_32 read_error = errno;
    log_file_warning(operation, path, "content"_view, "errno"_view, read_error);
    return {};
  }

  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(data));
}

static auto write_opened_file(
    FILE* file,
    View::Bytes data,
    View::Bytes operation,
    View::Bytes path) -> Bool {
  if (data.is_empty()) {
    return True;
  }

  CppSize items_written = fwrite(data.get_data(), data.get_size(), 1, file);
  if (items_written != 1) {
    Signed_32 write_error = errno;
    log_file_warning(
        operation, path, "content"_view, "errno"_view, write_error);
    return False;
  }

  return True;
}

static auto close_stream(FILE* file, View::Bytes operation, View::Bytes path)
    -> Bool {
  Signed_32 closed = fclose(file);
  if (closed == 0) {
    return True;
  }

  Signed_32 close_error = errno;
  log_file_warning(operation, path, "close"_view, "errno"_view, close_error);
  return False;
}

#ifdef PERI_LINUX
static auto close_descriptor(
    Signed_32 descriptor,
    View::Bytes operation,
    View::Bytes path) -> Bool {
  Signed_32 closed = close(descriptor);
  if (closed == 0) {
    return True;
  }

  Signed_32 close_error = errno;
  log_file_warning(operation, path, "close"_view, "errno"_view, close_error);
  return False;
}

static auto close_root_descriptor(Signed_32 descriptor) -> void {
  Signed_32 closed = close(descriptor);
  if (closed == 0) {
    return;
  }

  Signed_32 close_error = errno;
  Diagnostics::Log::Message<192> warning(
      Diagnostics::Log::Level::Warning, Diagnostics::Source());
  warning << "System::File::Root close failed. descriptor="_view << descriptor
          << " errno="_view << close_error;
}

static auto open_root_member(
    Signed_32 descriptor,
    const char* path,
    Unsigned_64 flags,
    Unsigned_64 mode = 0) -> Signed_32 {
  open_how policy = {
    .flags = flags,
    .mode = mode,
    .resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS,
  };
  return Signed_32(
      syscall(SYS_openat2, descriptor, path, &policy, sizeof(policy)));
}
#endif

// Writes one slash normalized path and exactly one final null into caller
// storage. The returned view excludes the terminator so it retains ordinary
// byte semantics.
static auto create_path(Access::Bytes output, View::Bytes path)
    -> Option<View::Bytes> {
  Count content_size = path.get_size();
  if (content_size != 0 && path[content_size - 1] == '\0') {
    content_size--;
  }

  for (Count i = 0; i < content_size; i++) {
    if (path[i] == '\0') {
      return {};
    }
  }

  if (content_size >= output.get_size()) {
    return {};
  }

  for (Count i = 0; i < content_size; i++) {
    output[i] = path[i] == '\\' ? '/' : path[i];
  }

  output[content_size] = '\0';
  return View::Bytes(output.get_data(), content_size);
}

static auto create_relative_path(Access::Bytes output, View::Bytes route)
    -> Option<View::Bytes> {
  auto native_route = create_path(output, route);
  if (!native_route || (*native_route).is_empty() ||
      (*native_route)[0] == '/') {
    return {};
  }

  Path normalized(*native_route);
  View::Bytes normalized_route = normalized.get_view();
  if (normalized_route.is_empty() || normalized.is_rooted()) {
    return {};
  }

  return create_path(output, normalized_route);
}

File::Root::Root(Signed_32 descriptor) : descriptor(descriptor) {}

File::Root::Root(Root&& source) : descriptor(source.descriptor) {
  source.descriptor = -1;
}

auto File::Root::operator=(Root&& source) -> Root& {
  if (this == &source) {
    return *this;
  }

#ifdef PERI_LINUX
  if (descriptor >= 0) {
    close_root_descriptor(descriptor);
  }
#else
#error Perimortem does not have a file implementation for this platform.
#endif

  descriptor = source.descriptor;
  source.descriptor = -1;
  return *this;
}

File::Root::~Root() {
#ifdef PERI_LINUX
  if (descriptor < 0) {
    return;
  }

  close_root_descriptor(descriptor);
  descriptor = -1;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::open(View::Bytes location) -> Option<Root> {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path || (*path).is_empty()) {
    return {};
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  Signed_32 descriptor =
      ::open(native_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (descriptor < 0) {
    return {};
  }

  return Option<Root>(Root(descriptor));
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::read(View::Bytes relative_path) const
    -> Option<Dynamic::Bytes> {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    log_file_warning(
        "System::File::Root read"_view, relative_path, "path"_view, "size"_view,
        Signed_64(relative_path.get_size()));
    return {};
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  Signed_32 member = open_root_member(
      descriptor, native_path, Unsigned_64(O_RDONLY | O_CLOEXEC));
  if (member < 0) {
    Signed_32 open_error = errno;
    log_file_warning(
        "System::File::Root read"_view, relative_path, "open"_view,
        "errno"_view, open_error);
    return {};
  }

  FILE* file = fdopen(member, "rb");
  if (!file) {
    Signed_32 stream_error = errno;
    log_file_warning(
        "System::File::Root read"_view, relative_path, "stream"_view,
        "errno"_view, stream_error);
    close_descriptor(member, "System::File::Root read"_view, relative_path);
    return {};
  }

  Option<Dynamic::Bytes> data =
      read_opened_file(file, "System::File::Root read"_view, relative_path);
  Bool closed =
      close_stream(file, "System::File::Root read"_view, relative_path);
  if (!closed) {
    return {};
  }

  return data;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::write(View::Bytes data, View::Bytes relative_path) const
    -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    log_file_warning(
        "System::File::Root write"_view, relative_path, "path"_view,
        "size"_view, Signed_64(relative_path.get_size()));
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  constexpr Unsigned_64 create_mode =
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  Signed_32 member = open_root_member(
      descriptor, native_path,
      Unsigned_64(O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC), create_mode);
  if (member < 0) {
    Signed_32 open_error = errno;
    log_file_warning(
        "System::File::Root write"_view, relative_path, "open"_view,
        "errno"_view, open_error);
    return False;
  }

  FILE* file = fdopen(member, "wb");
  if (!file) {
    Signed_32 stream_error = errno;
    log_file_warning(
        "System::File::Root write"_view, relative_path, "stream"_view,
        "errno"_view, stream_error);
    close_descriptor(member, "System::File::Root write"_view, relative_path);
    return False;
  }

  Bool written = write_opened_file(
      file, data, "System::File::Root write"_view, relative_path);
  Bool closed =
      close_stream(file, "System::File::Root write"_view, relative_path);
  return written && closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::remove(View::Bytes relative_path) const -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return False;
  }

  Count member_offset = 0;
  for (Count i = 0; i < (*path).get_size(); i++) {
    if ((*path)[i] == '/') {
      member_offset = i + 1;
    }
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  Signed_32 parent = descriptor;
  Bool close_parent = False;
  if (member_offset != 0) {
    path_buffer[member_offset - 1] = '\0';
    parent = open_root_member(
        descriptor, native_path, Unsigned_64(O_PATH | O_DIRECTORY | O_CLOEXEC));
    if (parent < 0) {
      return False;
    }

    close_parent = True;
  }

  const char* member_path = native_path + member_offset;
  Signed_32 removed = unlinkat(parent, member_path, 0);

  Bool parent_closed = True;
  if (close_parent) {
    parent_closed = close_descriptor(
        parent, "System::File::Root remove"_view, relative_path);
  }

  return removed == 0 && parent_closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::exists(View::Bytes relative_path) const -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  Signed_32 member = open_root_member(
      descriptor, native_path, Unsigned_64(O_PATH | O_CLOEXEC));
  if (member < 0) {
    return False;
  }

  struct stat64 status;
  Signed_32 status_read = fstat64(member, &status);
  Bool regular = status_read == 0 && S_ISREG(status.st_mode);

  Bool closed =
      close_descriptor(member, "System::File::Root exists"_view, relative_path);
  return regular && closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::read(View::Bytes location) -> Option<Dynamic::Bytes> {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    log_file_warning(
        "System::File read"_view, location, "path"_view, "size"_view,
        Signed_64(location.get_size()));
    return {};
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  FILE* file = fopen(native_path, "rb");
  if (!file) {
    Signed_32 open_error = errno;
    log_file_warning(
        "System::File read"_view, location, "open"_view, "errno"_view,
        open_error);
    return {};
  }

  Option<Dynamic::Bytes> data =
      read_opened_file(file, "System::File read"_view, location);
  Bool closed = close_stream(file, "System::File read"_view, location);
  if (!closed) {
    return {};
  }

  return data;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::write(View::Bytes data, View::Bytes location) -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    log_file_warning(
        "System::File write"_view, location, "path"_view, "size"_view,
        Signed_64(location.get_size()));
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

  FILE* file = fopen(native_path, "wb");
  if (!file) {
    Signed_32 open_error = errno;
    log_file_warning(
        "System::File write"_view, location, "open"_view, "errno"_view,
        open_error);
    return False;
  }

  Bool written =
      write_opened_file(file, data, "System::File write"_view, location);
  Bool closed = close_stream(file, "System::File write"_view, location);
  return written && closed;
}

auto File::remove(View::Bytes location) -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());
  Signed_32 removed = ::remove(native_path);
  return removed == 0;
}

auto File::exists(View::Bytes location) -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  struct stat64 status;
  Signed_32 status_read = stat64(native_path, &status);
  return status_read == 0 && Bool(status.st_mode & S_IFREG);
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}
