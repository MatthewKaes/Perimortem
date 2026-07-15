// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/package/buffer.hpp"

#include "tetrodotoxin/archiver/reader.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer;

Resolution::Package::Buffer::Buffer(View::Bytes content)
    : source(arena, content) {}

auto Resolution::Package::Buffer::read_manifest(
    Perimortem::Memory::Allocator::Arena& target) const
    -> const Tetrodotoxin::Archiver::Manifest* {
  return Tetrodotoxin::Archiver::Reader(source).read_manifest(target);
}

auto Resolution::Package::Buffer::read_package(
    Perimortem::Memory::Allocator::Arena& target,
    const Tetrodotoxin::Archiver::Manifest& manifest,
    Perimortem::Core::View::Vector<const Tetrodotoxin::Archiver::Package*>
        references) const -> Tetrodotoxin::Archiver::Package* {
  return Tetrodotoxin::Archiver::Reader(source).read_package(
      target, manifest, references);
}
