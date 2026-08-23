// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/unsigned.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Unsigned::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantUnsigned);
  BAIL_IF(!writer.write(get_type().get_name()));
  writer.write(get_value());
  return writer.finish(record);
}
