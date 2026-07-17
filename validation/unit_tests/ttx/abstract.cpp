// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Validation;

static Harness TtxAbstract = {
  .name = "Abstract"_view,
};

PERIMORTEM_UNIT_TEST(TtxAbstract, invalid_absorbs) {
  Invalid invalid;

  EXPECT_TEXT(invalid.get_name(), "Invalid"_view);
  EXPECT(&invalid.resolve() == &invalid);
  EXPECT(&invalid.resolve_context("Anything::Else"_view) == &invalid);
}

PERIMORTEM_UNIT_TEST(TtxAbstract, alias_reroutes) {
  /// A leaf proves that Alias routing preserves the target's resolution
  /// contract without adding a second model for values.
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

  /// A context owns the meaning of its routes. Alias only changes which
  /// context receives the borrowed route.
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
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "Use the palette context."_view,
    "Preserve authored color names."_view,
  }};
  Value color("Color"_view, invalid);
  Context graphics("Graphics"_view, "Color"_view, color, invalid);
  Alias palette("Palette"_view, graphics, Ttx::Concept::Documentation(lines));
  Alias colors("Colors"_view, palette);

  EXPECT_TEXT(palette.get_name(), "Palette"_view);
  EXPECT_EQ(palette.get_documentation().get_line_count(), Count(2));
  EXPECT_TEXT(
      palette.get_documentation().line_at(0), "Use the palette context."_view);
  EXPECT(colors.get_documentation().is_empty());
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
