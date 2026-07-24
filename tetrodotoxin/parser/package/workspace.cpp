// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/package/workspace.hpp"

#ifdef PERI_LINUX
#include <errno.h>
#include <fcntl.h>
#include <linux/openat2.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#else
#error Tetrodotoxin Package Workspace requires a confined host implementation.
#endif

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

using Failure = Parser::Package::Workspace::Failure;

static auto as_c_path(View::Bytes path) -> Dynamic::Bytes {
  Dynamic::Bytes terminated(path);
  terminated.append('\0');
  return terminated;
}

static auto classify_open_failure(int error) -> Failure {
  if (error == ENOENT || error == ENOTDIR) {
    return Failure::Missing;
  }
  if (error == EXDEV || error == ELOOP) {
    return Failure::OutsideRoot;
  }

  return Failure::Unreadable;
}

static auto valid_normalized_route(View::Bytes route) -> Bool {
  if (route.is_empty() || route[0] == '/' ||
      route[route.get_size() - 1] == '/') {
    return False;
  }

  Count segment_start = 0;
  for (Count i = 0; i <= route.get_size(); i++) {
    if (i < route.get_size() && route[i] != '/') {
      if (route[i] == '\\' || route[i] == '\0') {
        return False;
      }
      continue;
    }

    View::Bytes segment = route.slice(segment_start, i - segment_start);
    if (segment.is_empty() || segment == "."_view || segment == ".."_view) {
      return False;
    }
    segment_start = i + 1;
  }

  return True;
}

Parser::Package::Workspace::Open::Open(Open&& source) : result(source.result) {
  source.result = Failure::Unreadable;
}

auto Parser::Package::Workspace::Open::operator=(Open&& source) -> Open& {
  if (this == &source) {
    return *this;
  }

  release();
  result = source.result;
  source.result = Failure::Unreadable;
  return *this;
}

Parser::Package::Workspace::Open::~Open() {
  release();
}

auto Parser::Package::Workspace::Open::release() -> void {
  result.visit(
      []() {}, [](const Workspace& workspace) { delete &workspace; },
      [](Failure) {});
  result = Failure::Unreadable;
}

Parser::Package::Workspace::Read::Read(Construction, Dynamic::Bytes&& bytes) {
  Dynamic::Bytes& snapshot =
      *new Dynamic::Bytes(static_cast<Dynamic::Bytes&&>(bytes));
  result = snapshot;
}

Parser::Package::Workspace::Read::Read(Read&& source) : result(source.result) {
  source.result = Failure::Unreadable;
}

auto Parser::Package::Workspace::Read::operator=(Read&& source) -> Read& {
  if (this == &source) {
    return *this;
  }

  release();
  result = source.result;
  source.result = Failure::Unreadable;
  return *this;
}

Parser::Package::Workspace::Read::~Read() {
  release();
}

auto Parser::Package::Workspace::Read::release() -> void {
  result.visit(
      []() {}, [](Dynamic::Bytes& bytes) { delete &bytes; }, [](Failure) {});
  result = Failure::Unreadable;
}

Parser::Package::Workspace::~Workspace() {
  int closed = ::close(root);
  (void)closed;
}

auto Parser::Package::Workspace::open(View::Bytes package_root) -> Open {
  if (package_root.is_empty()) {
    return Open(Construction(), Failure::Missing);
  }

  Dynamic::Bytes path = as_c_path(package_root);
  const char* location = Data::cast<const char>(path.get_view().get_data());
  int root = ::open(location, O_PATH | O_CLOEXEC);
  if (root == -1) {
    return Open(Construction(), classify_open_failure(errno));
  }

  struct stat status{};
  int classified = ::fstat(root, &status);
  if (classified == -1) {
    int closed = ::close(root);
    (void)closed;
    return Open(Construction(), Failure::Unreadable);
  }
  if (!S_ISDIR(status.st_mode)) {
    int closed = ::close(root);
    (void)closed;
    return Open(Construction(), Failure::NonFile);
  }

  Workspace& workspace = *new Workspace(root);
  return Open(Construction(), workspace);
}

auto Parser::Package::Workspace::read(View::Bytes normalized_route) const
    -> Read {
  Bool valid = valid_normalized_route(normalized_route);
  if (!valid) {
    return Read(Construction(), Failure::OutsideRoot);
  }

  Dynamic::Bytes route = as_c_path(normalized_route);
  const char* location = Data::cast<const char>(route.get_view().get_data());
  struct open_how operation{};
  operation.flags = O_RDONLY | O_CLOEXEC;
  operation.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS;
  int file = int(
      ::syscall(SYS_openat2, root, location, &operation, sizeof(operation)));
  if (file == -1) {
    return Read(Construction(), classify_open_failure(errno));
  }

  struct stat status{};
  int classified = ::fstat(file, &status);
  if (classified == -1) {
    int closed = ::close(file);
    (void)closed;
    return Read(Construction(), Failure::Unreadable);
  }
  if (!S_ISREG(status.st_mode)) {
    int closed = ::close(file);
    (void)closed;
    return Read(Construction(), Failure::NonFile);
  }

  Dynamic::Bytes bytes;
  Static::Bytes<4096> chunk;
  while (True) {
    Signed_64 read_size =
        Signed_64(::read(file, chunk.get_data(), chunk.get_size()));
    if (read_size == -1 && errno == EINTR) {
      continue;
    }
    if (read_size == -1) {
      int closed = ::close(file);
      (void)closed;
      return Read(Construction(), Failure::Unreadable);
    }
    if (read_size == 0) {
      break;
    }

    bytes.concat(chunk.slice(0, Count(read_size)));
  }

  int closed = ::close(file);
  (void)closed;

  return Read(Construction(), static_cast<Dynamic::Bytes&&>(bytes));
}
