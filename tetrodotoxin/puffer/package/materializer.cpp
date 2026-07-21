// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/package/materializer.hpp"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer::Package;

static constexpr Count max_path_size = 512;

static auto convert_path(View::Bytes path, Unsigned_8* buffer) -> const char* {
  if (path.is_empty() || path.get_size() >= max_path_size) {
    return "";
  }

  for (Count i = 0; i < path.get_size(); i++) {
    buffer[i] = path[i];
  }
  buffer[path.get_size()] = '\0';
  return Perimortem::Core::Data::cast<const char>(buffer);
}

static auto directory_exists(View::Bytes path) -> Bool {
  Unsigned_8 buffer[max_path_size];
  const char* text = convert_path(path, buffer);
  if (text[0] == '\0') {
    return False;
  }

  struct stat status;
  return stat(text, &status) == 0 && S_ISDIR(status.st_mode);
}

static auto path_exists(View::Bytes path) -> Bool {
  Unsigned_8 buffer[max_path_size];
  const char* text = convert_path(path, buffer);
  if (text[0] == '\0') {
    return False;
  }

  struct stat status;
  return stat(text, &status) == 0;
}

static auto create_directory(
    View::Bytes path,
    Dynamic::Vector<Dynamic::Bytes>& created) -> Bool {
  if (directory_exists(path)) {
    return True;
  }

  Unsigned_8 buffer[max_path_size];
  const char* text = convert_path(path, buffer);
  if (text[0] == '\0' || mkdir(text, 0777) != 0) {
    return False;
  }

  created.insert(Dynamic::Bytes(path));
  return True;
}

static auto create_directories(
    View::Bytes path,
    Dynamic::Vector<Dynamic::Bytes>& created) -> Bool {
  for (Count i = 1; i <= path.get_size(); i++) {
    if (i < path.get_size() && path[i] != '/') {
      continue;
    }

    View::Bytes directory = path.slice(0, i);
    if (directory.get_size() == 1 && directory[0] == '/') {
      continue;
    }

    Bool made = create_directory(directory, created);
    if (!made) {
      return False;
    }
  }

  return True;
}

static auto remove_directory(View::Bytes path) -> void {
  Unsigned_8 buffer[max_path_size];
  const char* text = convert_path(path, buffer);
  if (text[0] != '\0') {
    rmdir(text);
  }
}

static auto clean_stage(
    Dynamic::Vector<Dynamic::Bytes>& files,
    Dynamic::Vector<Dynamic::Bytes>& directories) -> void {
  for (Count i = files.get_size(); i > 0; i--) {
    File::remove(files[i - 1]);
  }
  for (Count i = directories.get_size(); i > 0; i--) {
    remove_directory(directories[i - 1]);
  }
}

static auto clean_directories(Dynamic::Vector<Dynamic::Bytes>& directories)
    -> void {
  for (Count i = directories.get_size(); i > 0; i--) {
    remove_directory(directories[i - 1]);
  }
}

static auto publish_directory(View::Bytes stage, View::Bytes target) -> Bool {
  Unsigned_8 stage_buffer[max_path_size];
  Unsigned_8 target_buffer[max_path_size];
  const char* stage_text = convert_path(stage, stage_buffer);
  const char* target_text = convert_path(target, target_buffer);
  if (stage_text[0] == '\0' || target_text[0] == '\0') {
    return False;
  }

  return rename(stage_text, target_text) == 0;
}

static auto append_segment(Dynamic::Bytes& path, View::Bytes segment) -> void {
  if (!path.is_empty() && path[path.get_size() - 1] != '/') {
    path.append('/');
  }

  path.concat(segment);
}

auto Materializer::materialize(
    View::Bytes packages_root,
    const Tetrodotoxin::Archiver::Manifest& manifest,
    const Tetrodotoxin::Model::Packages::Compiled& package) -> Bool {
  Path normalized_root(packages_root);
  Allocator::Arena validation;
  if (!manifest.is_valid(validation) || normalized_root.get_view().is_empty()) {
    return False;
  }

  View::Vector<Tetrodotoxin::Model::Terminal> terminals =
      package.get_terminals();
  Managed::Map<View::Bytes, Bool> terminal_paths(validation);
  terminal_paths.ensure_capacity(terminals.get_size());
  for (Count i = 0; i < terminals.get_size(); i++) {
    if (!Tetrodotoxin::Model::Terminal::is_valid_path(
            terminals[i].get_path()) ||
        terminal_paths.find(terminals[i].get_path()) != nullptr) {
      return False;
    }

    terminal_paths.insert(terminals[i].get_path(), True);
  }

  Dynamic::Bytes package_directory(normalized_root.get_view());
  append_segment(package_directory, manifest.get_name());

  Dynamic::Bytes version;
  Stream::Textual<Dynamic::Bytes> version_writer(version);
  version_writer << manifest.get_version().get_major() << "."_view
                 << manifest.get_version().get_minor();

  Dynamic::Bytes target(package_directory);
  append_segment(target, version);
  Dynamic::Bytes stage(package_directory);
  stage.concat("/."_view);
  stage.concat(version);
  stage.concat(".pending"_view);
  if (path_exists(target) || path_exists(stage)) {
    return False;
  }

  Dynamic::Vector<Dynamic::Bytes> parent_directories;
  Bool made_parent = create_directories(package_directory, parent_directories);
  if (!made_parent) {
    clean_directories(parent_directories);
    return False;
  }

  Dynamic::Vector<Dynamic::Bytes> stage_directories;
  Dynamic::Vector<Dynamic::Bytes> stage_files;
  Bool made_stage = create_directories(stage, stage_directories);
  if (!made_stage) {
    clean_stage(stage_files, stage_directories);
    clean_directories(parent_directories);
    return False;
  }

  for (Count i = 0; i < terminals.get_size(); i++) {
    Dynamic::Bytes output(stage);
    append_segment(output, terminals[i].get_path());
    Path normalized_output(output);
    View::Bytes output_path = normalized_output.get_view();
    if (output_path.is_empty() ||
        output_path.slice(0, stage.get_size()) != stage.get_view() ||
        output_path.get_size() <= stage.get_size() ||
        output_path[stage.get_size()] != '/') {
      clean_stage(stage_files, stage_directories);
      clean_directories(parent_directories);
      return False;
    }

    Bool made_directories = create_directories(
        normalized_output.get_directory(), stage_directories);
    if (!made_directories) {
      clean_stage(stage_files, stage_directories);
      clean_directories(parent_directories);
      return False;
    }

    Bool wrote = File::write(terminals[i].get_content(), output_path);
    if (!wrote) {
      clean_stage(stage_files, stage_directories);
      clean_directories(parent_directories);
      return False;
    }

    stage_files.insert(Dynamic::Bytes(output_path));
  }

  Bool published = publish_directory(stage, target);
  if (!published) {
    clean_stage(stage_files, stage_directories);
    clean_directories(parent_directories);
    return False;
  }

  return True;
}
