// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/object.hpp"

#include "tetrodotoxin/library/language/types/object_storage.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Object::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantObject);
  auto object = get_type().select<Types::ObjectStorage>();
  BAIL_IF(
      !object || !writer.write(get_type().get_name()) ||
      !writer.write(object->get_element_type().get_name()));
  return writer.finish(record);
}
