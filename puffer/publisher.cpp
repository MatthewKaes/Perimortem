// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/publisher.hpp"

#include <atomic>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "tetrodotoxin/language/artifact_file.hpp"
#include "ttx/query.hpp"

using namespace Perimortem;

struct Product {
  Tetrodotoxin::Language::ArtifactObservation artifact;
};

class Descriptor {
 public:
  explicit Descriptor(int value = -1) : value(value) {}
  Descriptor(const Descriptor&) = delete;
  Descriptor(Descriptor&& source) noexcept
      : value(std::exchange(source.value, -1)) {}
  ~Descriptor() {
    if (value >= 0) {
      close(value);
    }
  }

  auto operator=(const Descriptor&) -> Descriptor& = delete;
  auto operator=(Descriptor&& source) noexcept -> Descriptor& {
    if (this != &source) {
      if (value >= 0) {
        close(value);
      }
      value = std::exchange(source.value, -1);
    }
    return *this;
  }

  explicit operator bool() const { return value >= 0; }
  auto get() const -> int { return value; }

 private:
  int value;
};

static auto text(Core::View::Bytes value) -> std::string {
  return std::string(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto ascii_alphanumeric(unsigned char byte) -> bool {
  return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
         (byte >= '0' && byte <= '9');
}

static auto ascii_upper(unsigned char byte) -> char {
  return byte >= 'a' && byte <= 'z' ? char(byte - ('a' - 'A')) : char(byte);
}

static auto ascii_lower(unsigned char byte) -> char {
  return byte >= 'A' && byte <= 'Z' ? char(byte + ('a' - 'A')) : char(byte);
}

static auto split(Core::View::Bytes route)
    -> std::optional<std::vector<std::string>> {
  if (route.is_empty() || route[0] == '/') {
    return std::nullopt;
  }
  std::vector<std::string> segments;
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); ++index) {
    if (index != route.get_size() && route[index] != '/') {
      continue;
    }
    Core::View::Bytes bytes = route.slice(start, index - start);
    if (bytes.is_empty()) {
      return std::nullopt;
    }
    std::string segment = text(bytes);
    const auto first = static_cast<unsigned char>(segment.front());
    if (!ascii_alphanumeric(first)) {
      return std::nullopt;
    }
    for (unsigned char byte : segment) {
      if (!ascii_alphanumeric(byte) && byte != '.' && byte != '_' &&
          byte != '+' && byte != '-') {
        return std::nullopt;
      }
    }
    if (segment == "." || segment == ".." || segment.back() == '.') {
      return std::nullopt;
    }

    std::string base;
    for (unsigned char byte : segment) {
      if (byte == '.') {
        break;
      }
      base.push_back(ascii_upper(byte));
    }
    const bool reserved =
        base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" ||
        (base.size() == 4 &&
         (base.compare(0, 3, "COM") == 0 || base.compare(0, 3, "LPT") == 0) &&
         base[3] >= '1' && base[3] <= '9');
    if (reserved) {
      return std::nullopt;
    }
    segments.push_back(std::move(segment));
    start = index + 1;
  }
  return segments;
}

static auto collision_key(const std::vector<std::string>& segments)
    -> std::string {
  std::string key;
  for (size_t index = 0; index < segments.size(); ++index) {
    if (index != 0) {
      key.push_back('/');
    }
    for (unsigned char byte : segments[index]) {
      key.push_back(ascii_lower(byte));
    }
  }
  return key;
}

static auto open_directory(int parent, const std::string& name, bool create)
    -> Descriptor {
  if (create &&
      mkdirat(
          parent, name.c_str(),
          S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 &&
      errno != EEXIST) {
    return Descriptor();
  }
  return Descriptor(openat(
      parent, name.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
}

static auto open_path(
    int root,
    const std::vector<std::string>& segments,
    size_t count,
    bool create) -> Descriptor {
  Descriptor current(dup(root));
  if (!current) {
    return Descriptor();
  }
  for (size_t index = 0; index < count; ++index) {
    Descriptor next = open_directory(current.get(), segments[index], create);
    if (!next) {
      return Descriptor();
    }
    current = std::move(next);
  }
  return current;
}

static auto write_all(int descriptor, const std::vector<uint8_t>& bytes)
    -> bool {
  size_t written = 0;
  while (written != bytes.size()) {
    const ssize_t result =
        write(descriptor, bytes.data() + written, bytes.size() - written);
    if (result < 0 && errno == EINTR) {
      continue;
    }
    if (result <= 0) {
      return false;
    }
    written += size_t(result);
  }
  return true;
}

static auto write_product(
    int stage,
    const Product& product,
    const std::vector<std::string>& route) -> bool {
  Descriptor parent = open_path(stage, route, route.size() - 1, true);
  if (!parent) {
    return false;
  }
  Descriptor file(openat(
      parent.get(), route.back().c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, S_IRUSR | S_IWUSR));
  if (!file || !write_all(file.get(), product.artifact.bytes)) {
    return false;
  }
  const mode_t mode = product.artifact.executable
                          ? S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
                          : S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  return fchmod(file.get(), mode) == 0 && fsync(file.get()) == 0 &&
         fsync(parent.get()) == 0;
}

static auto remove_contents(int directory) -> bool;

static auto remove_entry(int parent, const char* name) -> bool {
  struct stat status = {};
  if (fstatat(parent, name, &status, AT_SYMLINK_NOFOLLOW) != 0) {
    return errno == ENOENT;
  }
  if (!S_ISDIR(status.st_mode)) {
    return unlinkat(parent, name, 0) == 0;
  }
  Descriptor child(
      openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
  if (!child || !remove_contents(child.get())) {
    return false;
  }
  return unlinkat(parent, name, AT_REMOVEDIR) == 0;
}

static auto remove_contents(int directory) -> bool {
  const int duplicate = dup(directory);
  if (duplicate < 0) {
    return false;
  }
  DIR* stream = fdopendir(duplicate);
  if (stream == nullptr) {
    close(duplicate);
    return false;
  }
  bool removed = true;
  errno = 0;
  while (dirent* entry = readdir(stream)) {
    if (std::string(entry->d_name) == "." ||
        std::string(entry->d_name) == "..") {
      continue;
    }
    if (!remove_entry(directory, entry->d_name)) {
      removed = false;
      break;
    }
    errno = 0;
  }
  if (errno != 0) {
    removed = false;
  }
  return closedir(stream) == 0 && removed;
}

static auto commit(
    int parent,
    const std::string& stage,
    const std::string& target) -> bool {
  struct stat status = {};
  if (fstatat(parent, target.c_str(), &status, AT_SYMLINK_NOFOLLOW) != 0) {
    if (errno != ENOENT ||
        renameat(parent, stage.c_str(), parent, target.c_str()) != 0) {
      return false;
    }
    return fsync(parent) == 0;
  }
  if (!S_ISDIR(status.st_mode)) {
    return false;
  }
  const long exchanged = syscall(
      SYS_renameat2, parent, stage.c_str(), parent, target.c_str(),
      RENAME_EXCHANGE);
  if (exchanged != 0 || fsync(parent) != 0) {
    return false;
  }
  return remove_entry(parent, stage.c_str());
}

auto Puffer::Publisher::publish(ttx_pack products) const -> Bool {
  return publish(Core::View::Vector<ttx_pack>(&products, 1));
}

auto Puffer::Publisher::publish(Core::View::Vector<ttx_pack> products) const
    -> Bool {
  auto generation = split(publication_root);
  BAIL_IF(!generation || products.is_empty());

  std::vector<Product> retained;
  std::vector<std::vector<std::string>> routes;
  std::vector<std::string> collision_keys;
  for (ttx_pack products : products) {
    auto entries = Ttx::pack_entries(products);
    BAIL_IF(!entries || entries->empty());
    for (size_t index = 0; index < entries->size(); ++index) {
      for (size_t prior = 0; prior < index; ++prior) {
        BAIL_IF((*entries)[prior].path == (*entries)[index].path);
      }
      auto artifact =
          Tetrodotoxin::Language::observe_artifact((*entries)[index].producer);
      BAIL_IF(!artifact);
      auto route = split(
          Core::View::Bytes(artifact->route.data(), artifact->route.size()));
      BAIL_IF(!route);
      const std::string key = collision_key(*route);
      for (const std::string& prior : collision_keys) {
        BAIL_IF(prior == key);
      }
      collision_keys.push_back(key);
      routes.push_back(std::move(*route));
      retained.push_back({.artifact = std::move(*artifact)});
    }
  }

  const std::string root_path = text(root);
  Descriptor root_descriptor(
      open(root_path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
  BAIL_IF(!root_descriptor);
  Descriptor parent = open_path(
      root_descriptor.get(), *generation, generation->size() - 1, true);
  BAIL_IF(!parent);

  static std::atomic<uint64_t> sequence = 1;
  const std::string target = generation->back();
  const std::string stage =
      "." + target + ".puffer." + std::to_string(getpid()) + "." +
      std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)) + ".tmp";
  if (mkdirat(
          parent.get(), stage.c_str(),
          S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
    return False;
  }
  Descriptor stage_directory = open_directory(parent.get(), stage, false);
  if (!stage_directory) {
    remove_entry(parent.get(), stage.c_str());
    return False;
  }

  for (size_t index = 0; index < retained.size(); ++index) {
    if (!write_product(stage_directory.get(), retained[index], routes[index])) {
      stage_directory = Descriptor();
      remove_entry(parent.get(), stage.c_str());
      return False;
    }
  }
  if (fsync(stage_directory.get()) != 0) {
    stage_directory = Descriptor();
    remove_entry(parent.get(), stage.c_str());
    return False;
  }
  stage_directory = Descriptor();
  if (!commit(parent.get(), stage, target)) {
    remove_entry(parent.get(), stage.c_str());
    return False;
  }
  return True;
}
