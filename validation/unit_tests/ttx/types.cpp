// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/abstraction/alias.hpp"
#include "ttx/abstraction/invalid.hpp"
#include "ttx/model/types/flag.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Ttx::Model;
using namespace Ttx::Model::Types;
using namespace Validation;

static Harness TtxTypes = {
  .name = "TTX::Types"_view,
};

// A toolchain implementation owns the concrete terminal record and decides
// which contract instances participate in its resolution context.
template <typename Contract>
class RegisteredTerminal final : public Contract {
 public:
  RegisteredTerminal(
      View::Bytes name,
      Count size,
      Count alignment,
      const Invalid& invalid)
      : name(name), size(size), alignment(alignment), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_size() const -> Count override { return size; }
  auto get_alignment() const -> Count override { return alignment; }

 private:
  View::Bytes name;
  Count size;
  Count alignment;
  const Invalid& invalid;
};

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_is_a_queryable_type_contract) {
  Invalid invalid;
  RegisteredTerminal<Unsigned> unsigned_8(
      "Bits_8"_view, sizeof(::Bits_8), alignof(::Bits_8), invalid);
  const Abstract& abstract = unsigned_8;
  const Type& type = abstract.as<Type>();
  const Terminal& terminal = type.as<Terminal>();

  EXPECT(abstract.is<Abstract>());
  EXPECT(abstract.is<Type>());
  EXPECT(abstract.is<Terminal>());
  EXPECT(abstract.is<Unsigned>());
  EXPECT_NOT(abstract.is<Signed>());
  EXPECT_NOT(abstract.is<Real>());
  EXPECT_NOT(abstract.is<Flag>());
  EXPECT(type.is<Terminal>());
  EXPECT(terminal.is<Type>());
  EXPECT(&type == &terminal);
  EXPECT(&terminal == &abstract.as<Unsigned>());
  EXPECT(type.get_layout().is_empty());
  EXPECT_EQ(terminal.get_size(), Count(sizeof(::Bits_8)));
  EXPECT_EQ(terminal.get_alignment(), Count(alignof(::Bits_8)));
  EXPECT(abstract.resolve_context("Anything"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxTypes, families_are_distinct_terminal_contracts) {
  Invalid invalid;
  RegisteredTerminal<Unsigned> unsigned_type("Unsigned"_view, 4, 4, invalid);
  RegisteredTerminal<Signed> signed_type("Signed"_view, 4, 4, invalid);
  RegisteredTerminal<Real> real_type("Real"_view, 4, 4, invalid);
  RegisteredTerminal<Flag> flag_type("Flag"_view, 1, 1, invalid);

  EXPECT(unsigned_type.is<Unsigned>());
  EXPECT_NOT(unsigned_type.is<Signed>());
  EXPECT(signed_type.is<Signed>());
  EXPECT_NOT(signed_type.is<Unsigned>());
  EXPECT(real_type.is<Real>());
  EXPECT(flag_type.is<Flag>());
  EXPECT(unsigned_type.is<Terminal>());
  EXPECT(signed_type.is<Terminal>());
  EXPECT(real_type.is<Terminal>());
  EXPECT(flag_type.is<Terminal>());
}

PERIMORTEM_UNIT_TEST(TtxTypes, toolchain_selects_supported_widths) {
  Invalid invalid;
  RegisteredTerminal<Unsigned> unsigned_8(
      "Bits_8"_view, sizeof(::Bits_8), alignof(::Bits_8), invalid);
  RegisteredTerminal<Unsigned> unsigned_16(
      "Bits_16"_view, sizeof(::Bits_16), alignof(::Bits_16), invalid);
  RegisteredTerminal<Unsigned> unsigned_32(
      "Bits_32"_view, sizeof(::Bits_32), alignof(::Bits_32), invalid);
  RegisteredTerminal<Unsigned> unsigned_64(
      "Bits_64"_view, sizeof(::Bits_64), alignof(::Bits_64), invalid);
  RegisteredTerminal<Signed> signed_8(
      "Signed_8"_view, sizeof(::Signed_8), alignof(::Signed_8), invalid);
  RegisteredTerminal<Signed> signed_16(
      "Signed_16"_view, sizeof(::Signed_16), alignof(::Signed_16), invalid);
  RegisteredTerminal<Signed> signed_32(
      "Signed_32"_view, sizeof(::Signed_32), alignof(::Signed_32), invalid);
  RegisteredTerminal<Signed> signed_64(
      "Signed_64"_view, sizeof(::Signed_64), alignof(::Signed_64), invalid);
  RegisteredTerminal<Real> real_32(
      "Real_32"_view, sizeof(::Real_32), alignof(::Real_32), invalid);
  RegisteredTerminal<Real> real_64(
      "Real_64"_view, sizeof(::Real_64), alignof(::Real_64), invalid);
  RegisteredTerminal<Real> real_128(
      "Real_128"_view, sizeof(::Real_128), alignof(::Real_128), invalid);
  RegisteredTerminal<Flag> boolean(
      "Bool"_view, sizeof(::Bool), alignof(::Bool), invalid);
  Alias count("Count"_view, unsigned_64);
  const Abstract& cpp_size_type =
      sizeof(::CppSize) == sizeof(::Bits_64)
          ? static_cast<const Abstract&>(unsigned_64)
          : static_cast<const Abstract&>(unsigned_32);
  Alias cpp_size("CppSize"_view, cpp_size_type);

  EXPECT_EQ(unsigned_8.get_size(), Count(1));
  EXPECT_EQ(unsigned_16.get_size(), Count(2));
  EXPECT_EQ(unsigned_32.get_size(), Count(4));
  EXPECT_EQ(unsigned_64.get_size(), Count(8));
  EXPECT_EQ(signed_8.get_size(), Count(1));
  EXPECT_EQ(signed_16.get_size(), Count(2));
  EXPECT_EQ(signed_32.get_size(), Count(4));
  EXPECT_EQ(signed_64.get_size(), Count(8));
  EXPECT_EQ(real_32.get_size(), Count(4));
  EXPECT_EQ(real_64.get_size(), Count(8));
  EXPECT_EQ(real_128.get_size(), Count(16));
  EXPECT_EQ(boolean.get_size(), Count(1));
  EXPECT_EQ(unsigned_64.get_alignment(), Count(alignof(::Bits_64)));
  EXPECT_EQ(signed_64.get_alignment(), Count(alignof(::Signed_64)));
  EXPECT_EQ(real_128.get_alignment(), Count(alignof(::Real_128)));
  EXPECT_EQ(boolean.get_alignment(), Count(alignof(::Bool)));
  EXPECT(&count.resolve() == &unsigned_64);
  EXPECT_EQ(
      cpp_size.resolve().as<Terminal>().get_size(), Count(sizeof(::CppSize)));
}

PERIMORTEM_UNIT_TEST(TtxTypes, widths_are_instances_not_cpp_classes) {
  Invalid invalid;
  RegisteredTerminal<Unsigned> unsigned_24("Unsigned_24"_view, 3, 1, invalid);

  EXPECT(unsigned_24.is<Unsigned>());
  EXPECT(unsigned_24.is<Terminal>());
  EXPECT_EQ(unsigned_24.get_size(), Count(3));
  EXPECT_EQ(unsigned_24.get_alignment(), Count(1));
}
