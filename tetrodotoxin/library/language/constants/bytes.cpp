// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/bytes.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Bytes::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantBytes);
  BAIL_IF(
      !writer.write(get_type().get_name()) || !writer.write(get_value()) ||
      !writer.finish(record));
  return True;
}
