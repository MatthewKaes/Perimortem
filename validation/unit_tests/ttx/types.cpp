// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/boolean.hpp"
#include "ttx/model/types/real_128.hpp"
#include "ttx/model/types/real_32.hpp"
#include "ttx/model/types/real_64.hpp"
#include "ttx/model/types/signed_16.hpp"
#include "ttx/model/types/signed_32.hpp"
#include "ttx/model/types/signed_64.hpp"
#include "ttx/model/types/signed_8.hpp"
#include "ttx/model/types/unsigned_16.hpp"
#include "ttx/model/types/unsigned_32.hpp"
#include "ttx/model/types/unsigned_64.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

namespace Types = Ttx::Model::Types;

static Harness TtxTypes = {
  .name = "TTX::Types"_view,
};

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_contract) {
  Types::Unsigned_8 unsigned_8;
  const Abstract& abstract = unsigned_8;
  const Type& type = abstract.as<Type>();
  const Types::Terminal& terminal = type.as<Types::Terminal>();

  EXPECT(abstract.is<Abstract>());
  EXPECT(abstract.is<Type>());
  EXPECT(abstract.is<Types::Terminal>());
  EXPECT(abstract.is<Types::Unsigned>());
  EXPECT_NOT(abstract.is<Types::Signed>());
  EXPECT_NOT(abstract.is<Types::Real>());
  EXPECT_NOT(abstract.is<Types::Flag>());
  EXPECT(type.is<Types::Terminal>());
  EXPECT(terminal.is<Type>());
  EXPECT(&type == &terminal);
  EXPECT(&terminal == &abstract.as<Types::Unsigned>());
  EXPECT(type.get_layout().is_empty());
  EXPECT_EQ(terminal.get_width(), Count(8));
  EXPECT_EQ(terminal.get_size(), Count(sizeof(::Unsigned_8)));
  EXPECT_EQ(terminal.get_alignment(), Count(alignof(::Unsigned_8)));
  EXPECT_NOT(terminal.get_documentation().is_empty());
  EXPECT(abstract.resolve_context("Anything"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_families) {
  Types::Unsigned_32 unsigned_type;
  Types::Signed_32 signed_type;
  Types::Real_32 real_type;
  Types::Boolean flag_type;

  EXPECT(unsigned_type.is<Types::Unsigned>());
  EXPECT_NOT(unsigned_type.is<Types::Signed>());
  EXPECT(signed_type.is<Types::Signed>());
  EXPECT_NOT(signed_type.is<Types::Unsigned>());
  EXPECT(real_type.is<Types::Real>());
  EXPECT(flag_type.is<Types::Flag>());
  EXPECT(unsigned_type.is<Types::Terminal>());
  EXPECT(signed_type.is<Types::Terminal>());
  EXPECT(real_type.is<Types::Terminal>());
  EXPECT(flag_type.is<Types::Terminal>());
}

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_abi) {
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Unsigned_32 unsigned_32;
  Types::Unsigned_64 unsigned_64;
  Types::Signed_8 signed_8;
  Types::Signed_16 signed_16;
  Types::Signed_32 signed_32;
  Types::Signed_64 signed_64;
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Types::Real_128 real_128;
  Types::Boolean boolean;
  EXPECT_EQ(unsigned_8.get_size(), Count(sizeof(::Unsigned_8)));
  EXPECT_EQ(unsigned_16.get_size(), Count(sizeof(::Unsigned_16)));
  EXPECT_EQ(unsigned_32.get_size(), Count(sizeof(::Unsigned_32)));
  EXPECT_EQ(unsigned_64.get_size(), Count(sizeof(::Unsigned_64)));
  EXPECT_EQ(signed_8.get_size(), Count(sizeof(::Signed_8)));
  EXPECT_EQ(signed_16.get_size(), Count(sizeof(::Signed_16)));
  EXPECT_EQ(signed_32.get_size(), Count(sizeof(::Signed_32)));
  EXPECT_EQ(signed_64.get_size(), Count(sizeof(::Signed_64)));
  EXPECT_EQ(real_32.get_size(), Count(sizeof(::Real_32)));
  EXPECT_EQ(real_64.get_size(), Count(sizeof(::Real_64)));
  EXPECT_EQ(real_128.get_size(), Count(sizeof(::Real_128)));
  EXPECT_EQ(boolean.get_size(), Count(sizeof(::Bool)));

  EXPECT_EQ(unsigned_8.get_alignment(), Count(alignof(::Unsigned_8)));
  EXPECT_EQ(unsigned_16.get_alignment(), Count(alignof(::Unsigned_16)));
  EXPECT_EQ(unsigned_32.get_alignment(), Count(alignof(::Unsigned_32)));
  EXPECT_EQ(unsigned_64.get_alignment(), Count(alignof(::Unsigned_64)));
  EXPECT_EQ(signed_8.get_alignment(), Count(alignof(::Signed_8)));
  EXPECT_EQ(signed_16.get_alignment(), Count(alignof(::Signed_16)));
  EXPECT_EQ(signed_32.get_alignment(), Count(alignof(::Signed_32)));
  EXPECT_EQ(signed_64.get_alignment(), Count(alignof(::Signed_64)));
  EXPECT_EQ(real_32.get_alignment(), Count(alignof(::Real_32)));
  EXPECT_EQ(real_64.get_alignment(), Count(alignof(::Real_64)));
  EXPECT_EQ(real_128.get_alignment(), Count(alignof(::Real_128)));
  EXPECT_EQ(boolean.get_alignment(), Count(alignof(::Bool)));

  EXPECT_EQ(unsigned_8.get_width(), Count(sizeof(::Unsigned_8) * 8));
  EXPECT_EQ(unsigned_16.get_width(), Count(sizeof(::Unsigned_16) * 8));
  EXPECT_EQ(unsigned_32.get_width(), Count(sizeof(::Unsigned_32) * 8));
  EXPECT_EQ(unsigned_64.get_width(), Count(sizeof(::Unsigned_64) * 8));
  EXPECT_EQ(signed_8.get_width(), Count(sizeof(::Signed_8) * 8));
  EXPECT_EQ(signed_16.get_width(), Count(sizeof(::Signed_16) * 8));
  EXPECT_EQ(signed_32.get_width(), Count(sizeof(::Signed_32) * 8));
  EXPECT_EQ(signed_64.get_width(), Count(sizeof(::Signed_64) * 8));
  EXPECT_EQ(real_32.get_width(), Count(sizeof(::Real_32) * 8));
  EXPECT_EQ(real_64.get_width(), Count(sizeof(::Real_64) * 8));
  EXPECT_EQ(real_128.get_width(), Count(sizeof(::Real_128) * 8));
  EXPECT_EQ(boolean.get_width(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_aliases) {
  Types::Unsigned_32 unsigned_32;
  Types::Unsigned_64 unsigned_64;
  Alias count("Count"_view, unsigned_64);
  const Abstract& cpp_size_type =
      sizeof(::CppSize) == sizeof(::Unsigned_64)
          ? static_cast<const Abstract&>(unsigned_64)
          : static_cast<const Abstract&>(unsigned_32);
  Alias cpp_size("CppSize"_view, cpp_size_type);

  EXPECT(&count.resolve() == &unsigned_64);
  EXPECT_EQ(
      cpp_size.resolve().as<Types::Terminal>().get_size(),
      Count(sizeof(::CppSize)));
}

PERIMORTEM_UNIT_TEST(TtxTypes, terminal_names) {
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Unsigned_32 unsigned_32;
  Types::Unsigned_64 unsigned_64;
  Types::Signed_8 signed_8;
  Types::Signed_16 signed_16;
  Types::Signed_32 signed_32;
  Types::Signed_64 signed_64;
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Types::Real_128 real_128;
  Types::Boolean boolean;

  EXPECT_TEXT(unsigned_8.get_name(), "Unsigned_8"_view);
  EXPECT_TEXT(unsigned_16.get_name(), "Unsigned_16"_view);
  EXPECT_TEXT(unsigned_32.get_name(), "Unsigned_32"_view);
  EXPECT_TEXT(unsigned_64.get_name(), "Unsigned_64"_view);
  EXPECT_TEXT(signed_8.get_name(), "Signed_8"_view);
  EXPECT_TEXT(signed_16.get_name(), "Signed_16"_view);
  EXPECT_TEXT(signed_32.get_name(), "Signed_32"_view);
  EXPECT_TEXT(signed_64.get_name(), "Signed_64"_view);
  EXPECT_TEXT(real_32.get_name(), "Real_32"_view);
  EXPECT_TEXT(real_64.get_name(), "Real_64"_view);
  EXPECT_TEXT(real_128.get_name(), "Real_128"_view);
  EXPECT_TEXT(boolean.get_name(), "Bool"_view);
  EXPECT_TEXT(
      real_32.get_documentation().get_line(0),
      "Real_32 is stored as a 4 byte IEEE floating value."_view);
  EXPECT_TEXT(
      real_64.get_documentation().get_line(0),
      "Real_64 is stored as an 8 byte IEEE floating value."_view);
}
