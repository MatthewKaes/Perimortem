// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/product.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Language::Product::create(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    Core::View::Bytes value) -> Product& {
  Core::View::Bytes retained_name = arena.proxy(name);
  Core::View::Bytes retained_value = arena.proxy(value);
  return arena.construct_from<Product>(
      [&]() -> Product { return Product(retained_name, retained_value); });
}

auto Language::Product::value_abi(const ttx_abstract* identity)
    -> perimortem_bytes {
  const auto& product =
      static_cast<const Product&>(Ttx::Concept::Abstract::from_abi(identity));
  return {
    .data = product.get_value().get_data(),
    .size = product.get_value().get_size(),
  };
}

const tetrodotoxin_product_operations Language::Product::product_operations = {
  .interface = {.negotiate = tetrodotoxin_product_relation},
  .value = value_abi,
};

auto Language::Product::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == tetrodotoxin_product_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &product_operations.interface)
             : Ttx::Concept::Constant::negotiate_interface(requirement);
}
