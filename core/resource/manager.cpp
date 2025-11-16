// Perimortem Engine
// Copyright © Matt Kaes

#include "resource/manager.hpp"

#include "storage/virtual_disk_writer.hpp"

#include <array>
#include <bit>
#include <chrono>
#include <fstream>

using namespace Perimortem::Storage;
using namespace Perimortem::Resource;

auto get_time(const std::filesystem::path& p) -> Resource::Time {
  if (std::filesystem::exists(p))
    return std::filesystem::last_write_time(p).time_since_epoch().count();

  // Microseconds is realistically overkill and not even available on most
  // systems, but might as well try to use it.
  return Resource::Time();
}

auto Resource::read_content() const -> const ByteView {
  if (!content.empty())
    return {content.data(), content.size()};

  return source;
}

auto Resource::write_content(const Bytes&& data) -> void {
  if (flags == StorageOptions::ReadOnly) {
    return;
  }

  content = std::move(data);
  dirty = true;
  loaded = true;
  time = std::chrono::file_clock::now().time_since_epoch().count();
}

Manager::Manager(const std::filesystem::path& data_root_) {
  data_root = data_root_;

  // Load script virtual disk if it exists.

  for (int i = 0; i < Path::logical_disks.size(); i++) {
    std::filesystem::path sector_disk = data_root / Path::logical_disks[i];
    sector_disk.replace_extension(virutal_disk_extension);

    // No disk exists for this sector.
    if (!sectors[i].create(sector_disk))
      continue;

    for (const auto& file : sectors[i].reader->get_files()) {
      Path p = Path(static_cast<Path::Sector>(i), file.path);
      directory.insert({p, std::make_unique<Resource>(Resource())});
      auto& res = directory.at(p);
      res->content.clear();
      res->flags = file.options;
      res->dirty = false;
      res->loaded = false;

      if (res->flags == StorageOptions::Virtualized) {
        res->source = file.data;
        res->dirty = false;
        res->loaded = true;
        res->time = std::chrono::file_clock::now().time_since_epoch().count();
      } else if (res->flags == StorageOptions::Preload) {
        load_file(p, *res);
      }
    }
  }
}

Manager::~Manager() {
#ifdef PERI_EDITOR
  flush_changes();
#endif
}

auto Manager::lookup(const Path& p) -> Resource* {
  // Unknown file
  if (!exists(p))
    return nullptr;

  // Manage ondemand files.
  auto& res = *directory.at(p);
  load_file(p, res);
  return directory.at(p).get();
}

auto Manager::exists(const Path& p) -> bool {
  return directory.contains(p);
}

auto Manager::move(const Path& src, const Path& dest) -> bool {
  if (src == dest)
    return false;

  if (!exists(src))
    return false;

  if (exists(dest))
    return false;

  auto handle = directory.extract(src);
  handle.key() = dest;
  directory.insert(std::move(handle));
  auto& res = *directory.at(dest);
  res.time = std::chrono::file_clock::now().time_since_epoch().count();

  if (res.flags != StorageOptions::Virtualized) {
    std::filesystem::path origin_src = data_root / src.get_path();
    std::filesystem::path origin_dest = data_root / dest.get_path();
    if (std::filesystem::exists(origin_src)) {
      std::filesystem::create_directories(origin_dest.parent_path());
      std::filesystem::copy(origin_src, origin_dest);
      std::filesystem::remove(origin_src);
    }
  }

  return true;
}

auto Manager::copy(const Path& src, const Path& dest) -> bool {
  if (src == dest)
    return false;

  if (!exists(src))
    return false;

  if (exists(dest))
    return false;

  auto& res = *directory.at(src);
  load_file(src, res);
  directory.insert({dest, res.clone()});
  directory.at(dest)->time =
      std::chrono::file_clock::now().time_since_epoch().count();

  if (res.flags != StorageOptions::Virtualized) {
    std::filesystem::path origin_src = data_root / src.get_path();
    std::filesystem::path origin_dest = data_root / dest.get_path();
    if (std::filesystem::exists(origin_src)) {
      std::filesystem::create_directories(origin_dest.parent_path());
      std::filesystem::copy(origin_src, origin_dest);
    }
  }

  return true;
}

auto Manager::remove(const Path& p) -> bool {
  if (!exists(p))
    return false;

  if (directory.at(p)->flags != StorageOptions::Virtualized) {
    std::filesystem::path origin =
        data_root / std::filesystem::path(p.get_path());
    if (std::filesystem::exists(origin))
      if (!std::filesystem::remove(origin))
        return false;
  }

  directory.erase(p);
  return true;
}

auto Manager::create(const Path& p, Bytes&& data, StorageFlags options)
    -> Resource* {
  if (directory.contains(p))
    return nullptr;

  directory.insert({p, std::make_unique<Resource>(Resource())});
  auto& res = directory.at(p);
  res->content = std::move(data);
  res->time = std::chrono::file_clock::now().time_since_epoch().count();
  res->flags = options;
  res->dirty = true;
  res->loaded = true;

  flush_file(p);
  return directory.at(p).get();
}

auto Manager::import(const Path& p,
                     std::filesystem::path src,
                     StorageFlags options) -> Resource* {
  if (directory.contains(p))
    return nullptr;

  std::error_code ec;
  if (!std::filesystem::is_regular_file(src, ec)) {
    // TODO: Error messages
    return nullptr;
  }

  std::ifstream source_stream(src, std::ios::binary);
  if (!source_stream.is_open()) {
    // TODO: Error messages
    return nullptr;
  }

  source_stream.seekg(0, std::ios_base::end);
  size_t length = source_stream.tellg();
  source_stream.seekg(0, std::ios_base::beg);

  // New data
  directory.insert({p, std::make_unique<Resource>(Resource())});
  auto& res = directory.at(p);
  res->content.clear();
  res->content.reserve(length);
  res->flags = options;
  res->dirty = true;
  res->loaded = true;
  res->time = std::chrono::file_clock::now().time_since_epoch().count();
  std::copy(std::istreambuf_iterator<char>(source_stream),
            std::istreambuf_iterator<char>(), std::back_inserter(res->content));

  return res.get();
}

auto Manager::change_file_flags(const Path& p, StorageFlags options) -> bool {
  if (!directory.contains(p))
    return false;

  auto& res = directory.at(p);
  res->flags = options;

  load_file(p, *res);
  return true;
}

auto Manager::set_sector_type(Path::Sector sector, Storage::DiskType type)
    -> void {
  sectors[(int)sector].type = type;
}

auto Manager::load_file(const Path& p, Resource& res) -> void {
  // Nothing to do.
  if (res.is_virtual() && res.in_memory())
    return;

  if (res.flags == StorageOptions::Streamed) {
    if (res.in_memory())
      return;

    // The file requested streaming but we don't have a disk to load it from.
    auto& disk = sectors[(int)p.get_sector()].reader;
    if (!disk) {
      // TODO: Error handling
      __builtin_debugtrap();
    }

    if (!disk->stream_from_disk(p.get_path(), res.content)) {
      // TODO: Error handling
      __builtin_debugtrap();
    }

    res.loaded = true;
    res.dirty = false;
    return;
  }

  // Load the file from origin.
  std::filesystem::path origin =
      data_root / std::filesystem::path(p.get_path());
  std::error_code ec;
  if (!std::filesystem::is_regular_file(origin, ec)) {
    // If the file doesn't exists but it's in memory then it's most likely an
    // import or currently a virtual file waiting for a flush.
    if (res.in_memory())
      return;

    // TODO: Error messages
    __builtin_debugtrap();
    return;
  }

  // We have a newer version in memory than disk.
  if (res.loaded && get_time(origin) <= res.time)
    return;

  // Load the file via a file stream.
  std::ifstream source_stream(origin, std::ios::binary);
  if (!source_stream.is_open()) {
    // TODO: Error messages
    __builtin_debugtrap();
    return;
  }

  source_stream.seekg(0, std::ios_base::end);
  size_t length = source_stream.tellg();
  source_stream.seekg(0, std::ios_base::beg);

  // New data
  res.content.clear();
  res.content.reserve(length);
  std::copy(std::istreambuf_iterator<char>(source_stream),
            std::istreambuf_iterator<char>(), std::back_inserter(res.content));

  res.time = get_time(origin);
  res.loaded = true;
  res.dirty = false;
}

auto Manager::flush_file(const Path& p) -> void {
  if (!exists(p))
    return;

  // Don't flush the file out if:
  // - It's virtual
  // - It has no changes (save disk writes)
  // - The file was never loaded (don't wipe the file)
  auto& res = *directory.at(p);
  const auto& bytes = res.read_content();
  std::filesystem::path origin =
      data_root / std::filesystem::path(p.get_path());
  if (res.is_virtual() || !res.dirty || !res.loaded) {
    return;
  }

  // We have the file and our version on disk is equal to or older than what we
  // have.
  if (std::filesystem::exists(origin) && res.time < get_time(origin)) {
    return;
  }

  // Ensure directory structure.
  std::filesystem::create_directories(origin.parent_path());

  std::ofstream source_stream(origin, std::ios::binary);
  if (!source_stream.is_open()) {
    // TODO: Error handling.
    __builtin_debugtrap();
  }

  source_stream.write((const char*)bytes.data(), bytes.size() * sizeof(Byte));
  res.dirty = false;  // Data is now flushed
}

auto Manager::flush_changes() -> void {
  // If we are only referencing a file on disk and have a change of it in
  // memory then we can flush it out to disk. Useful for save files.
  for (const auto& entry : directory)
    flush_file(entry.first);

  // Number of virtualized tables
  VirtualDiskWriter vtables[Path::sector_count];
  for (int i = 0; i < Path::sector_count; i++) {
    vtables[i].create(sectors[i].type);
  }

  bool has_data[Path::sector_count] = {false};
  for (const auto& entry : directory) {
    has_data[(int)entry.first.get_sector()] = true;

    auto content = ByteView();
    if (entry.second->is_virtual())
      content = entry.second->read_content();

    VirtualDiskWriter& table = vtables[(int32_t)entry.first.get_sector()];
    table.write_resource(entry.first.get_origin(), content.data(),
                         content.size(), entry.second->flags);
  }

  // create disks but ignore empty disks.
  for (int i = 0; i < Path::sector_count; i++) {
    if (has_data[i]) {
      vtables[i].write_disk(data_root / Path::logical_disks[i]);
    }
  }
}

auto Manager::SectorData::create(const std::filesystem::path& autogenetic)
    -> bool {
  type = DiskType::Standard;
  if (!std::filesystem::is_regular_file(autogenetic))
    return false;

  reader = Storage::VirtualDiskReader::mount_disk(autogenetic);

  // provided a file
  if (!reader)
    return false;

  type = reader->get_format();
  return true;
}