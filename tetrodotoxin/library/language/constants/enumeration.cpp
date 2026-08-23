// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/enumeration.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Enumeration::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantEnumeration);
  BAIL_IF(!writer.write(get_type().get_name()));
  writer.write(get_value());
  return writer.finish(record);
}
