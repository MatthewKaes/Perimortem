// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/core/prelude.hpp"
#include "ttx/layout.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxCore = {
  .name = "Ttx::Core"_view,
};

PERIMORTEM_UNIT_TEST(TtxCore, package_types) {
  const Ttx::Type* boolean = Ttx::Core::Prelude::find_type("Bool"_view);
  ASSERT(boolean);
  EXPECT_TEXT(boolean->get_name(), "Bool"_view);
  EXPECT(boolean->equivalent_to(*Ttx::Core::Prelude::find_type("Bool"_view)));
  EXPECT_NOT(
      boolean->equivalent_to(*Ttx::Core::Prelude::find_type("Bits_8"_view)));
  EXPECT(Ttx::Layout(*boolean).is_empty());
  EXPECT(Ttx::Layout(*boolean).equivalent_to(
      Ttx::Layout(*Ttx::Core::Prelude::find_type("Bits_8"_view))));

  const Ttx::Type* size_2d = Ttx::Core::Prelude::find_type("Size2D"_view);
  ASSERT(size_2d);
  Ttx::Layout layout(*size_2d);
  EXPECT_NOT(layout.is_empty());
  EXPECT_EQ(layout.get_member_count(), 2);
}

PERIMORTEM_UNIT_TEST(TtxCore, prelude_types) {
  EXPECT(Ttx::Core::Prelude::is_type("Void"_view));
  EXPECT(Ttx::Core::Prelude::find_type("Void"_view));
}

PERIMORTEM_UNIT_TEST(TtxCore, type_lookup) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(real_32);

  Ttx::Type sprite("Sprite"_view);
  Static::Vector<Ttx::Type::Member, 1> members = {
    Ttx::Type::Member{"size"_view, *real_32},
  };
  Static::Vector<const Ttx::Type*, 1> types = {
    &sprite,
  };
  Static::Vector<Ttx::Type::Member, 1> parameters = {
    Ttx::Type::Member{"value"_view, *real_32},
  };
  Static::Vector<Ttx::Type::Function, 1> functions = {
    Ttx::Type::Function{
        "format"_view, parameters.get_view(), parameters.get_view()},
  };
  Ttx::Type drawable(
      "Drawable"_view,
      members.get_view(),
      types.get_view(),
      functions.get_view());

  const Ttx::Type::Member* size = drawable.find_member("size"_view);
  ASSERT(size);
  EXPECT(size->get_type() == real_32);
  EXPECT(drawable.find_type("Sprite"_view) == &sprite);
  EXPECT(drawable.find_function("format"_view));
  EXPECT_NOT(drawable.find_member("missing"_view));
  EXPECT_NOT(drawable.find_type("missing"_view));
  EXPECT_NOT(drawable.find_function("missing"_view));
}

PERIMORTEM_UNIT_TEST(TtxCore, type_duplicates) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  const Ttx::Type* bool_type = Ttx::Core::Prelude::find_type("Bool"_view);
  ASSERT(real_32);
  ASSERT(bool_type);

  Ttx::Type sprite_a("Sprite"_view);
  Ttx::Type sprite_b("Sprite"_view);
  Static::Vector<Ttx::Type::Member, 2> members = {
    Ttx::Type::Member{"size"_view, *real_32},
    Ttx::Type::Member{"size"_view, *bool_type},
  };
  Static::Vector<const Ttx::Type*, 2> types = {
    &sprite_a,
    &sprite_b,
  };
  Static::Vector<Ttx::Type::Function, 2> functions = {
    Ttx::Type::Function{
        "format"_view,
        View::Vector<Ttx::Type::Member>(),
        View::Vector<Ttx::Type::Member>()},
    Ttx::Type::Function{
        "format"_view,
        View::Vector<Ttx::Type::Member>(),
        View::Vector<Ttx::Type::Member>()},
  };
  Ttx::Type duplicates(
      "Duplicates"_view,
      members.get_view(),
      types.get_view(),
      functions.get_view());

  const Ttx::Type::Member* size = duplicates.find_member("size"_view);
  ASSERT(size);
  EXPECT(size->get_type() == real_32);
  EXPECT(duplicates.find_type("Sprite"_view) == &sprite_a);
  EXPECT(duplicates.find_function("format"_view) == &functions[0]);
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_order) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(real_32);

  Static::Vector<Ttx::Type::Member, 2> xy_members = {
    Ttx::Type::Member{"x"_view, *real_32},
    Ttx::Type::Member{"y"_view, *real_32},
  };
  Static::Vector<Ttx::Type::Member, 2> yx_members = {
    Ttx::Type::Member{"y"_view, *real_32},
    Ttx::Type::Member{"x"_view, *real_32},
  };

  Ttx::Layout xy(xy_members.get_view());
  Ttx::Layout yx(yx_members.get_view());
  EXPECT(xy.equivalent_to(xy));
  EXPECT_NOT(xy.equivalent_to(yx));
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_duplicates) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  const Ttx::Type* bool_type = Ttx::Core::Prelude::find_type("Bool"_view);
  ASSERT(real_32);
  ASSERT(bool_type);

  Static::Vector<Ttx::Type::Member, 2> duplicate_members = {
    Ttx::Type::Member{"x"_view, *real_32},
    Ttx::Type::Member{"x"_view, *bool_type},
  };
  Static::Vector<Ttx::Type::Member, 2> duplicate_copy_members = {
    Ttx::Type::Member{"x"_view, *real_32},
    Ttx::Type::Member{"x"_view, *bool_type},
  };
  Static::Vector<Ttx::Type::Member, 2> xy_members = {
    Ttx::Type::Member{"x"_view, *real_32},
    Ttx::Type::Member{"y"_view, *bool_type},
  };

  Ttx::Layout duplicate(duplicate_members.get_view());
  Ttx::Layout duplicate_copy(duplicate_copy_members.get_view());
  Ttx::Layout xy(xy_members.get_view());
  const Ttx::Type::Member* first = duplicate.find_member("x"_view);
  ASSERT(first);
  EXPECT(first->get_type() == real_32);
  EXPECT(duplicate.equivalent_to(duplicate_copy));
  EXPECT_NOT(duplicate.equivalent_to(xy));
  EXPECT_NOT(xy.equivalent_to(duplicate));
  EXPECT(duplicate.fits(duplicate_copy));
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_fit) {
  const Ttx::Type* size_2d = Ttx::Core::Prelude::find_type("Size2D"_view);
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(size_2d);
  ASSERT(real_32);

  Static::Vector<Ttx::Type::Member, 2> named_members = {
    Ttx::Type::Member{"height"_view, *real_32},
    Ttx::Type::Member{"width"_view, *real_32},
  };
  Ttx::Layout named(named_members.get_view());
  EXPECT(named.fits(Ttx::Layout(*size_2d)));

  Static::Vector<Ttx::Type::Member, 2> positional_members = {
    Ttx::Type::Member{View::Bytes(), *real_32},
    Ttx::Type::Member{View::Bytes(), *real_32},
  };
  Ttx::Layout positional(positional_members.get_view());
  EXPECT(positional.fits(Ttx::Layout(*size_2d)));
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_wrong_type) {
  const Ttx::Type* size_2d = Ttx::Core::Prelude::find_type("Size2D"_view);
  const Ttx::Type* boolean = Ttx::Core::Prelude::find_type("Bool"_view);
  ASSERT(size_2d);
  ASSERT(boolean);

  Static::Vector<Ttx::Type::Member, 2> named_members = {
    Ttx::Type::Member{"width"_view, *boolean},
    Ttx::Type::Member{"height"_view, *boolean},
  };
  Ttx::Layout named(named_members.get_view());
  EXPECT_NOT(named.fits(Ttx::Layout(*size_2d)));
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_suffix) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(real_32);

  Static::Vector<Ttx::Type::Member, 2> target_members = {
    Ttx::Type::Member{"width"_view, *real_32},
    Ttx::Type::Member{"height"_view, *real_32, True},
  };
  Ttx::Type size_with_default(
      "SizeWithDefault"_view, target_members.get_view());

  Static::Vector<Ttx::Type::Member, 1> source_members = {
    Ttx::Type::Member{"width"_view, *real_32},
  };
  Ttx::Layout source(source_members.get_view());
  EXPECT(source.fits(Ttx::Layout(size_with_default)));
}

PERIMORTEM_UNIT_TEST(TtxCore, layout_default_gap) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(real_32);

  Static::Vector<Ttx::Type::Member, 3> target_members = {
    Ttx::Type::Member{"width"_view, *real_32},
    Ttx::Type::Member{"height"_view, *real_32, True},
    Ttx::Type::Member{"depth"_view, *real_32},
  };
  Ttx::Type size_with_default_gap(
      "SizeWithDefaultGap"_view, target_members.get_view());

  Static::Vector<Ttx::Type::Member, 3> source_members = {
    Ttx::Type::Member{"width"_view, *real_32},
    Ttx::Type::Member{"height"_view, *real_32},
    Ttx::Type::Member{"depth"_view, *real_32},
  };
  Ttx::Layout source(source_members.get_view());
  EXPECT_NOT(source.fits(Ttx::Layout(size_with_default_gap)));
}

PERIMORTEM_UNIT_TEST(TtxCore, alias_canonical) {
  const Ttx::Type* real_32 = Ttx::Core::Prelude::find_type("Real_32"_view);
  ASSERT(real_32);

  Ttx::Type scalar = Ttx::Type::alias("Scalar"_view, *real_32);
  Ttx::Type scalar_alias = Ttx::Type::alias("ScalarAlias"_view, scalar);
  EXPECT(scalar.equivalent_to(*real_32));
  EXPECT(scalar_alias.equivalent_to(*real_32));
}
