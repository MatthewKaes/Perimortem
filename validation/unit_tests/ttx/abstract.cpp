// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/block.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Documentations;
using namespace Validation;

static Harness TtxAbstract = {
  .name = "Abstract"_view,
};

PERIMORTEM_UNIT_TEST(TtxAbstract, invalid_absorbs) {
  const Invalid& invalid = Invalid::get_invalid();

  EXPECT_TEXT(invalid.get_name(), "Invalid"_view);
  EXPECT(&invalid == &Invalid::get_invalid());
  EXPECT(&invalid.resolve() == &invalid);
  EXPECT(&invalid.resolve_context("Anything::Else"_view) == &invalid);
  EXPECT(invalid.get_documentation().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxAbstract, alias_reroutes) {
  /// A leaf proves that Alias routing preserves the target's resolution
  /// contract without adding a second model for values.
  class Value : public Abstract {
   public:
    Value(
        View::Bytes name,
        const Documentation& documentation = Documentation::get_empty())
        : name(name), documentation(documentation) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes) const -> const Abstract& override {
      return Invalid::get_invalid();
    }
    auto get_documentation() const -> const Documentation& override {
      return documentation;
    }

   private:
    View::Bytes name;
    const Documentation& documentation;
  };

  /// A context owns the meaning of its routes. Alias only changes which
  /// context receives the borrowed route.
  class Context : public Abstract {
   public:
    Context(
        View::Bytes name,
        View::Bytes child_name,
        const Abstract& child,
        const Documentation& documentation = Documentation::get_empty())
        : name(name),
          child_name(child_name),
          child(child),
          documentation(documentation) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve_context(View::Bytes route) const -> const Abstract& override {
      if (route == child_name) {
        return child.resolve();
      }
      return Invalid::get_invalid();
    }
    auto get_documentation() const -> const Documentation& override {
      return documentation;
    }

   private:
    View::Bytes name;
    View::Bytes child_name;
    const Abstract& child;
    const Documentation& documentation;
  };

  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "Use the palette context."_view,
    "Preserve authored color names."_view,
  }};
  static constexpr Comment graphics_comment("The graphics context."_view);
  static constexpr Block palette_comments(lines);
  static constexpr Merged palette_documentation(
      palette_comments, graphics_comment);
  Value color("Color"_view);
  Context graphics("Graphics"_view, "Color"_view, color, graphics_comment);
  Alias palette("Palette"_view, graphics, palette_documentation);
  Alias colors("Colors"_view, palette);

  EXPECT_TEXT(palette.get_name(), "Palette"_view);
  EXPECT_EQ(palette.get_documentation().line_count(), Count(3));
  EXPECT_TEXT(
      palette.get_documentation().get_line(0), "Use the palette context."_view);
  EXPECT_TEXT(
      palette.get_documentation().get_line(2), "The graphics context."_view);
  EXPECT_EQ(colors.get_documentation().line_count(), Count(3));
  EXPECT_TEXT(
      colors.get_documentation().get_line(2), "The graphics context."_view);
  EXPECT_TEXT(palette.resolve().get_name(), "Graphics"_view);
  EXPECT(&palette.resolve() == &graphics);
  EXPECT(&colors.resolve() == &colors.resolve().resolve());
  EXPECT(&palette.resolve_context("Color"_view) == &color);
  EXPECT(
      &palette.resolve_context("Color"_view) ==
      &palette.resolve_context("Color"_view));
  EXPECT(&colors.resolve_context("Color"_view) == &color);
  EXPECT(&palette.resolve_context("Missing"_view) == &Invalid::get_invalid());
}
