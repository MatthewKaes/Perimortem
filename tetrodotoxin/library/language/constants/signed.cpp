// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/signed.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Signed::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantSigned);
  BAIL_IF(!writer.write(get_type().get_name()));
  writer.write(get_value());
  return writer.finish(record);
}

auto Language::Constants::Signed::lower(Llvm::Builder& body) const -> Bool {
  return prepare_carrier(body) &&
         body.signed_value(get_type(), *this, get_value());
}
