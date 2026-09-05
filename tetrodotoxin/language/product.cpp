// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/product.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

Language::Product::Product(
    Core::View::Bytes name,
    Core::View::Bytes value,
    Bool executable)
    : name(name), value(value), executable(executable) {}

auto Language::Product::create(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    Core::View::Bytes value,
    Bool executable) -> Product& {
  Core::View::Bytes retained_name = arena.proxy(name);
  Core::View::Bytes retained_value = arena.proxy(value);
  return arena.construct_from<Product>([&]() -> Product {
    return Product(retained_name, retained_value, executable);
  });
}
