// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/range.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Range::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantRange);
  BAIL_IF(
      !writer.write(get_type().get_name()) ||
      !writer.write(get_type().get_element_type().get_name()) ||
      !writer.finish(record));
  return True;
}

auto Language::Constants::Range::lower(Llvm::Builder& body) const -> Bool {
  return prepare_carrier(body) && body.empty_range(get_type(), *this);
}
