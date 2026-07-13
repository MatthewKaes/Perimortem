// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/source/record.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer;

Resolution::Source::Record::Record(
    View::Bytes source_path,
    View::Bytes source_text)
    : source_path(retain(source_path)), source_text(retain(source_text)) {
  Path path(this->source_path);
  View::Bytes file = path.get_file();
  View::Bytes extension = path.get_extension();
  module = retain(file.slice(0, file.get_size() - extension.get_size()));
}

Resolution::Source::Record::Record(View::Bytes source_path)
    : Record(source_path, View::Bytes()) {}

auto Resolution::Source::Record::complete(
    const Tetrodotoxin::Isa::Dialect& dialect,
    View::Vector<Tetrodotoxin::Puffer::Isa::Boot::Import> imports,
    const Ttx::Type& type) -> Bool {
  if (is_complete() || !dialect.is_valid() || type.is_invalid()) {
    return False;
  }

  this->dialect = dialect;
  this->imports = imports;
  this->type = &type;
  return True;
}

auto Resolution::Source::Record::retain(View::Bytes bytes) -> View::Bytes {
  if (bytes.is_empty()) {
    return View::Bytes();
  }

  Bits_8* data = arena.allocate(bytes.get_size());
  Data::copy(data, bytes.get_data(), bytes.get_size());
  return View::Bytes(data, bytes.get_size());
}
