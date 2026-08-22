// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/real.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Real::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantReal);
  BAIL_IF(!writer.write(get_type().get_name()));
  writer.write(get_value());
  return writer.finish(record);
}

auto Language::Constants::Real::lower(Llvm::Builder& body) const -> Bool {
  return prepare_carrier(body) &&
         body.real_value(get_type(), *this, get_value());
}
