// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/flag.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Flag::persist(Archive::Writer& writer) const -> Bool {
  auto tag =
      get_value() ? Archive::Tag::ConstantTrue : Archive::Tag::ConstantFalse;
  auto record = writer.begin(tag);
  BAIL_IF(!writer.write(get_type().get_name()) || !writer.finish(record));
  return True;
}
