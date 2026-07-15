// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/package.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/archiver/reader.hpp"
#include "tetrodotoxin/archiver/writer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Validation;

static Harness PufferPackageBench = {
  .name = "Puffer::Package"_view,
};

static constexpr View::Bytes math_path =
    ".bin/bin/tetrodotoxin/standard/Perimortem.Math/"
    "binary_archive.puffer"_view;
static constexpr View::Bytes graphics_path =
    ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/"
    "binary_archive.puffer"_view;

PERIMORTEM_BENCHMARK(PufferPackageBench, package_manifest) {
  auto graphics_buffer = File::read(graphics_path);
  if (graphics_buffer.is_empty()) {
    return;
  }

  Count accumulator = 0;
  Allocator::Arena arena;
  Benchmark::start_time();
  for (Count i = 0; i < 128; i++) {
    arena.reset();
    Tetrodotoxin::Archiver::Reader reader(graphics_buffer);
    const Manifest* manifest = reader.read_manifest(arena);
    if (manifest != nullptr) {
      accumulator += manifest->get_name().get_size();
      accumulator += manifest->get_imports().get_size();
    }
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(PufferPackageBench, package_restore) {
  auto math_buffer = File::read(math_path);
  auto graphics_buffer = File::read(graphics_path);
  if (math_buffer.is_empty() || graphics_buffer.is_empty()) {
    return;
  }

  Count accumulator = 0;
  Benchmark::start_time();
  for (Count i = 0; i < 16; i++) {
    Allocator::Arena arena;
    Tetrodotoxin::Archiver::Reader math_reader(math_buffer);
    const Manifest* math_manifest = math_reader.read_manifest(arena);
    if (math_manifest == nullptr) {
      continue;
    }

    const Package* math = math_reader.read_package(
        arena, *math_manifest, View::Vector<const Package*>());
    if (math == nullptr) {
      continue;
    }

    Tetrodotoxin::Archiver::Reader graphics_reader(graphics_buffer);
    const Manifest* graphics_manifest = graphics_reader.read_manifest(arena);
    if (graphics_manifest == nullptr) {
      continue;
    }

    View::Vector<Dependency> imports = graphics_manifest->get_imports();
    Dynamic::Vector<const Package*> references;
    for (Count k = 0; k < imports.get_size(); k++) {
      if (imports[k].get_source_name() == "Perimortem.Math"_view) {
        references.insert(math);
      }
    }

    const Package* graphics =
        graphics_reader.read_package(arena, *graphics_manifest, references);
    if (graphics != nullptr) {
      accumulator += graphics->get_type().get_types().get_size();
      accumulator += graphics->get_types().get_size();
    }
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(PufferPackageBench, package_write) {
  auto math_buffer = File::read(math_path);
  auto graphics_buffer = File::read(graphics_path);
  if (math_buffer.is_empty() || graphics_buffer.is_empty()) {
    return;
  }

  Allocator::Arena package_arena;
  Tetrodotoxin::Archiver::Reader math_reader(math_buffer);
  const Manifest* math_manifest = math_reader.read_manifest(package_arena);
  if (math_manifest == nullptr) {
    return;
  }

  const Package* math = math_reader.read_package(
      package_arena, *math_manifest, View::Vector<const Package*>());
  if (math == nullptr) {
    return;
  }

  Tetrodotoxin::Archiver::Reader graphics_reader(graphics_buffer);
  const Manifest* graphics_manifest =
      graphics_reader.read_manifest(package_arena);
  if (graphics_manifest == nullptr) {
    return;
  }

  View::Vector<Dependency> imports = graphics_manifest->get_imports();
  Dynamic::Vector<const Package*> references;
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_source_name() == "Perimortem.Math"_view) {
      references.insert(math);
    }
  }

  const Package* graphics = graphics_reader.read_package(
      package_arena, *graphics_manifest, references);
  if (graphics == nullptr) {
    return;
  }

  Count accumulator = 0;
  Allocator::Arena output_arena;
  Benchmark::start_time();
  for (Count i = 0; i < 16; i++) {
    output_arena.reset();
    View::Bytes output = Tetrodotoxin::Archiver::Writer::write(
        output_arena, graphics->get_manifest().get_name(),
        graphics->get_manifest().get_imports(), graphics->get_type(),
        graphics->get_types(), graphics->get_terminals(),
        graphics->get_linkages(), references);
    accumulator += output.get_size();
  }

  Benchmark::prevent_optimization(accumulator);
}
