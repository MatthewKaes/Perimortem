// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/source/storage.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer;

Resolution::Source::Storage::Storage(
    View::Bytes source_path,
    View::Bytes source_text)
    : source_path(retain(source_path)), source_text(retain(source_text)) {
  Path path(this->source_path);
  View::Bytes file = path.get_file();
  View::Bytes extension = path.get_extension();
  module = retain(file.slice(0, file.get_size() - extension.get_size()));
}

Resolution::Source::Storage::Storage(View::Bytes source_path)
    : Storage(source_path, View::Bytes()) {}

auto Resolution::Source::Storage::retain(View::Bytes bytes) -> View::Bytes {
  if (bytes.is_empty()) {
    return View::Bytes();
  }

  Bits_8* data = arena.allocate(bytes.get_size());
  Data::copy(data, bytes.get_data(), bytes.get_size());
  return View::Bytes(data, bytes.get_size());
}
