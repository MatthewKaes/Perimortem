// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/bytes.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Bytes::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantBytes);
  BAIL_IF(
      !writer.write(get_type().get_name()) || !writer.write(get_value()) ||
      !writer.finish(record));
  return True;
}

auto Language::Constants::Bytes::lower(Llvm::Builder& body) const -> Bool {
  return prepare_carrier(body) &&
         body.bytes_value(get_type(), *this, get_value());
}
