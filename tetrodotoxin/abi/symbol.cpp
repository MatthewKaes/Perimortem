// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/abi/symbol.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

static constexpr Bits_64 fnv_prime = 1099511628211ull;
static constexpr Bits_64 high_seed = 14695981039346656037ull;
static constexpr Bits_64 low_seed = 7809847782465536322ull;

static auto append(Bits_64& high, Bits_64& low, View::Bytes value) -> void {
  Count size = value.get_size();
  for (Count i = 0; i < sizeof(Count); i++) {
    Bits_8 byte = Bits_8(size >> (i * 8));
    high = (high ^ byte) * fnv_prime;
    low = (low ^ byte) * fnv_prime;
  }

  for (Count i = 0; i < value.get_size(); i++) {
    high = (high ^ value[i]) * fnv_prime;
    low = (low ^ value[i]) * fnv_prime;
  }
}

static auto append(Bits_64& high, Bits_64& low, Bits_64 value) -> void {
  Static::Bytes<sizeof(Bits_64)> bytes;
  for (Count i = 0; i < bytes.get_size(); i++) {
    bytes[i] = Bits_8(value >> (i * 8));
  }

  append(high, low, bytes);
}

static auto append(Bits_64& high, Bits_64& low, const Ttx::Type& type) -> void {
  append(high, low, type.resolve_attribute("cpp"_view).get_bytes());
  append(high, low, type.resolve_attribute("abi"_view).get_unsigned());
  append(high, low, type.canonical().get_name());
}

static auto append(Bits_64& high, Bits_64& low, const Ttx::Function& function)
    -> void {
  append(high, low, function.get_name());

  View::Vector<Ttx::Member> parameters =
      function.get_parameters().get_members();
  append(high, low, "Parameters"_view);
  for (Count i = 0; i < parameters.get_size(); i++) {
    append(high, low, parameters[i].get_type());
  }

  View::Vector<Ttx::Member> results = function.get_result().get_members();
  append(high, low, "Results"_view);
  for (Count i = 0; i < results.get_size(); i++) {
    append(high, low, results[i].get_type());
  }
}

static auto append(
    Bits_64& high,
    Bits_64& low,
    const Ttx::Type& type,
    View::Vector<Abi::Type> type_identities) -> Bool {
  append(high, low, type.resolve_attribute("cpp"_view).get_bytes());
  append(high, low, type.resolve_attribute("abi"_view).get_unsigned());

  const Ttx::Type& canonical = type.canonical();
  for (Count i = 0; i < type_identities.get_size(); i++) {
    if (&type_identities[i].get_type() == &canonical) {
      append(high, low, type_identities[i].get_path());
      return True;
    }
  }

  return False;
}

static auto append(
    Bits_64& high,
    Bits_64& low,
    const Ttx::Function& function,
    View::Vector<Abi::Type> type_identities) -> Bool {
  append(high, low, function.get_name());

  View::Vector<Ttx::Member> parameters =
      function.get_parameters().get_members();
  append(high, low, "Parameters"_view);
  for (Count i = 0; i < parameters.get_size(); i++) {
    Bool appended =
        append(high, low, parameters[i].get_type(), type_identities);
    if (!appended) {
      return False;
    }
  }

  View::Vector<Ttx::Member> results = function.get_result().get_members();
  append(high, low, "Results"_view);
  for (Count i = 0; i < results.get_size(); i++) {
    Bool appended = append(high, low, results[i].get_type(), type_identities);
    if (!appended) {
      return False;
    }
  }

  return True;
}

static auto build_symbol(
    Allocator::Arena& arena,
    View::Bytes prefix,
    Bits_64 high,
    Bits_64 low) -> View::Bytes {
  constexpr View::Bytes digits = "0123456789abcdef"_view;
  Managed::Bytes output(arena);
  output.concat(prefix);
  for (Signed_32 shift = 60; shift >= 0; shift -= 4) {
    output.append(digits[(high >> shift) & 0x0F]);
  }

  for (Signed_32 shift = 60; shift >= 0; shift -= 4) {
    output.append(digits[(low >> shift) & 0x0F]);
  }

  return output.get_view();
}

static auto build_internal_symbol(
    Allocator::Arena& arena,
    View::Bytes unit,
    View::Bytes module,
    View::Bytes table,
    View::Bytes owner_path,
    const Ttx::Function& function) -> View::Bytes {
  Bits_64 high = high_seed;
  Bits_64 low = low_seed;
  append(high, low, "Tetrodotoxin internal symbol"_view);
  append(high, low, unit);
  append(high, low, module);
  append(high, low, owner_path);
  append(high, low, table);
  append(high, low, function);
  return build_symbol(arena, "ttx_internal_"_view, high, low);
}

auto Abi::Symbol::type(
    Allocator::Arena& arena,
    View::Bytes unit,
    View::Bytes module,
    View::Bytes owner_path,
    const Ttx::Function& function) -> View::Bytes {
  return build_internal_symbol(
      arena, unit, module, "Type"_view, owner_path, function);
}

auto Abi::Symbol::addressable(
    Allocator::Arena& arena,
    View::Bytes unit,
    View::Bytes module,
    View::Bytes owner_path,
    const Ttx::Function& function) -> View::Bytes {
  return build_internal_symbol(
      arena, unit, module, "Addressable"_view, owner_path, function);
}

auto Abi::Symbol::exported(
    Allocator::Arena& arena,
    View::Bytes public_path,
    const Ttx::Function& function,
    View::Vector<Abi::Type> type_identities) -> View::Bytes {
  if (public_path.is_empty() || function.is_empty()) {
    return View::Bytes();
  }

  Bits_64 high = high_seed;
  Bits_64 low = low_seed;
  append(high, low, "Tetrodotoxin exported symbol"_view);
  append(high, low, public_path);
  Bool appended = append(high, low, function, type_identities);
  if (!appended) {
    return View::Bytes();
  }

  return build_symbol(arena, "ttx_"_view, high, low);
}
