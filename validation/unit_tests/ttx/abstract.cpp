// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/alias.hpp"
#include "ttx/invalid.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAbstract = {
  .name = "TTX::Abstract"_view,
};

PERIMORTEM_UNIT_TEST(TtxAbstract, invalid_is_absorbing) {
  Ttx::Invalid invalid;

  EXPECT_TEXT(invalid.get_name(), "Invalid"_view);
  EXPECT(&invalid.resolve() == &invalid);
  EXPECT(&invalid.resolve_context("Anything::Else"_view) == &invalid);
}

PERIMORTEM_UNIT_TEST(TtxAbstract, alias_reroutes_context) {
  class Value : public Ttx::Abstract {
   public:
    Value(View::Bytes name, const Ttx::Invalid& invalid)
        : name(name), invalid(invalid) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes) const -> const Ttx::Abstract& override {
      return invalid;
    }

   private:
    View::Bytes name;
    const Ttx::Invalid& invalid;
  };

  class Context : public Ttx::Abstract {
   public:
    Context(
        View::Bytes name,
        View::Bytes child_name,
        const Ttx::Abstract& child,
        const Ttx::Invalid& invalid)
        : name(name), child_name(child_name), child(child), invalid(invalid) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes route) const
        -> const Ttx::Abstract& override {
      if (route == child_name) {
        return child.resolve();
      }
      return invalid;
    }

   private:
    View::Bytes name;
    View::Bytes child_name;
    const Ttx::Abstract& child;
    const Ttx::Invalid& invalid;
  };

  Ttx::Invalid invalid;
  Value color("Color"_view, invalid);
  Context graphics("Graphics"_view, "Color"_view, color, invalid);
  Ttx::Alias palette("Palette"_view, graphics);
  Ttx::Alias colors("Colors"_view, palette);

  EXPECT_TEXT(palette.get_name(), "Palette"_view);
  EXPECT_TEXT(palette.resolve().get_name(), "Graphics"_view);
  EXPECT(&palette.resolve() == &graphics);
  EXPECT(&colors.resolve() == &colors.resolve().resolve());
  EXPECT(&palette.resolve_context("Color"_view) == &color);
  EXPECT(
      &palette.resolve_context("Color"_view) ==
      &palette.resolve_context("Color"_view));
  EXPECT(&colors.resolve_context("Color"_view) == &color);
  EXPECT(&palette.resolve_context("Missing"_view) == &invalid);
}
