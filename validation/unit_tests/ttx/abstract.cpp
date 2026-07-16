// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/abstraction/alias.hpp"
#include "ttx/abstraction/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Validation;

static Harness TtxAbstract = {
  .name = "Abstract"_view,
};

PERIMORTEM_UNIT_TEST(TtxAbstract, invalid_is_absorbing) {
  Invalid invalid;

  EXPECT_TEXT(invalid.get_name(), "Invalid"_view);
  EXPECT(&invalid.resolve() == &invalid);
  EXPECT(&invalid.resolve_context("Anything::Else"_view) == &invalid);
}

PERIMORTEM_UNIT_TEST(TtxAbstract, alias_reroutes_context) {
  class Value : public Abstract {
   public:
    Value(View::Bytes name, const Invalid& invalid)
        : name(name), invalid(invalid) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes) const -> const Abstract& override {
      return invalid;
    }

   private:
    View::Bytes name;
    const Invalid& invalid;
  };

  class Context : public Abstract {
   public:
    Context(
        View::Bytes name,
        View::Bytes child_name,
        const Abstract& child,
        const Invalid& invalid)
        : name(name), child_name(child_name), child(child), invalid(invalid) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes route) const -> const Abstract& override {
      if (route == child_name) {
        return child.resolve();
      }
      return invalid;
    }

   private:
    View::Bytes name;
    View::Bytes child_name;
    const Abstract& child;
    const Invalid& invalid;
  };

  Invalid invalid;
  Value color("Color"_view, invalid);
  Context graphics("Graphics"_view, "Color"_view, color, invalid);
  Alias palette("Palette"_view, graphics);
  Alias colors("Colors"_view, palette);

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
