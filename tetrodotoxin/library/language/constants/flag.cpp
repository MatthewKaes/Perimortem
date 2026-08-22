// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/flag.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Flag::persist(Archive::Writer& writer) const -> Bool {
  auto tag =
      get_value() ? Archive::Tag::ConstantTrue : Archive::Tag::ConstantFalse;
  auto record = writer.begin(tag);
  BAIL_IF(!writer.write(get_type().get_name()) || !writer.finish(record));
  return True;
}

auto Language::Constants::Flag::lower(Llvm::Builder& body) const -> Bool {
  return prepare_carrier(body) &&
         body.unsigned_value(get_type(), *this, U64(bool(get_value())));
}
