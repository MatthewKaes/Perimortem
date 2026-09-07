// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/reference.hpp"

#include "ttx/query.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem;

Reference::Reference(ttx_abstract host, Core::View::Bytes name)
    : host(host), name(name) {}

auto Reference::create(
    Memory::Allocator::Arena& arena,
    ttx_abstract host,
    Core::View::Bytes name) -> Reference& {
  return arena.construct<Reference>(host, name);
}

auto Reference::resolve(ttx_abstract) const -> ttx_abstract {
  const auto context = Ttx::resolve(host);
  return Ttx::resolve(
      Ttx::resolve_concept(context, {name.get_data(), name.get_size()}));
}

auto Reference::get_name() const -> Core::View::Bytes {
  return name;
}
auto Reference::get_host() const -> ttx_abstract {
  return host;
}
