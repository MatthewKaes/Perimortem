// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/compiler/execution.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;
using namespace Validation;

static Harness CompilerConstant = {
  .name = "Compiler::Constant"_view,
};

PERIMORTEM_UNIT_TEST(CompilerConstant, alternatives) {
  Execution::Constant bits(Bits_64(42));
  ASSERT(bits.find<Bits_64>() != nullptr);
  EXPECT_EQ(*bits.find<Bits_64>(), Bits_64(42));
  EXPECT(bits.find<Real_64>() == nullptr);

  Execution::Constant signed_bits(Signed_64(-42));
  ASSERT(signed_bits.find<Signed_64>() != nullptr);
  EXPECT_EQ(*signed_bits.find<Signed_64>(), Signed_64(-42));

  Execution::Constant real(Real_64(4.25));
  ASSERT(real.find<Real_64>() != nullptr);
  EXPECT_EQ(*real.find<Real_64>(), Real_64(4.25));

  Execution::Constant bytes("content"_view);
  ASSERT(bytes.find<View::Bytes>() != nullptr);
  EXPECT_TEXT(*bytes.find<View::Bytes>(), "content"_view);

  Execution::Constant flag(True);
  ASSERT(flag.find<Bool>() != nullptr);
  EXPECT(*flag.find<Bool>());
}
