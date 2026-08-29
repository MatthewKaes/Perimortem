// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/constant.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/context.hpp"
#include "ttx/model/documentations/block.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

static Harness TtxAbstract = {
  .name = "Abstract"_view,
};

PERIMORTEM_UNIT_TEST(TtxAbstract, unknown_is_provisional) {
  Memory::Allocator::Arena arena;
  Ttx::Model::Context context(arena);
  const Unknown& unknown = Unknown::get_unknown();

  EXPECT_TEXT(unknown.get_name(), "Unknown"_view);
  EXPECT(&unknown == &Unknown::get_unknown());
  EXPECT(&unknown.resolve() == &unknown);
  EXPECT(&unknown.get_type() == &unknown);
  EXPECT(&unknown.resolve_concept("Anything::Else"_view) == &unknown);
  EXPECT(unknown.get_concepts(context).get_layout().is_empty());
  EXPECT(unknown.get_documentation().is_empty());
  EXPECT_NOT(unknown.is<Constant>());
}

PERIMORTEM_UNIT_TEST(TtxAbstract, none_is_axiomatic) {
  const None& none = None::get_none();

  EXPECT_TEXT(none.get_name(), "None"_view);
  EXPECT(none.is<Constant>());
  EXPECT(&none.resolve() == &none);
  EXPECT(&none.get_type() == &none);
  EXPECT(&none.resolve_concept("Anything"_view) == &none);
}

PERIMORTEM_UNIT_TEST(TtxAbstract, contract_visit) {
  const Unknown& invalid = Unknown::get_unknown();
  Alias alias("Failure"_view, invalid);
  const Abstract& selected = alias;

  Bool matched = selected.visit<Alias>(
      [&](const Alias& value) { return &value == &alias ? True : False; },
      [](const Abstract&) { return False; });
  Bool rejected = invalid.visit<Alias>(
      [](const Alias&) { return False; },
      [&](const Abstract& value) { return &value == &invalid ? True : False; });

  EXPECT(matched);
  EXPECT(rejected);
}

PERIMORTEM_UNIT_TEST(TtxAbstract, reference_cv) {
  const Unknown& invalid = Unknown::get_unknown();
  Alias alias("Failure"_view, invalid);
  Reference<Alias> mutable_reference(alias);
  Reference<const Alias> read_reference(alias);
  Abstract& mutable_selected = alias;
  const Abstract& read_selected = alias;
  auto mutable_alias = mutable_selected.select<Alias>();
  auto read_alias = read_selected.select<Alias>();
  auto rejected_alias = invalid.select<Alias>();

  static_assert(__is_same(decltype(mutable_reference.get()), Alias&));
  static_assert(__is_same(decltype(read_reference.get()), const Alias&));
  static_assert(__is_same(decltype(mutable_alias), Option<Alias&>));
  static_assert(__is_same(decltype(read_alias), Option<const Alias&>));

  EXPECT(&mutable_reference.get() == &alias);
  EXPECT(&read_reference.get() == &alias);
  EXPECT(mutable_alias && &*mutable_alias == &alias);
  EXPECT(read_alias && &*read_alias == &alias);
  EXPECT_NOT(rejected_alias);
  EXPECT(mutable_selected.visit<Alias>(
      [&](Alias& value) { return &value == &alias ? True : False; },
      [](Abstract&) { return False; }));
  EXPECT(read_selected.visit<Alias>(
      [&](const Alias& value) { return &value == &alias ? True : False; },
      [](const Abstract&) { return False; }));
}

PERIMORTEM_UNIT_TEST(TtxAbstract, alias_binding) {
  /// A leaf that resolves elsewhere proves Alias returns the first non Alias
  /// identity without observing the terminal owner's completion contract.
  class Value : public Abstract {
   public:
    Value(
        View::Bytes name,
        const Documentation& documentation = Documentation::get_empty())
        : name(name), documentation(documentation) {}

    auto get_name() const -> View::Bytes override { return name; }
    auto resolve() const -> const Abstract& override {
      return Unknown::get_unknown();
    }
    auto resolve_concept(View::Bytes) const -> const Abstract& override {
      return Unknown::get_unknown();
    }
    auto get_documentation() const -> const Documentation& override {
      return documentation;
    }

   private:
    View::Bytes name;
    const Documentation& documentation;
  };

  /// A context owns the meaning of its routes. A consumer must first resolve
  /// an Alias, then ask that exact context explicitly.
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
    auto resolve_concept(View::Bytes route) const -> const Abstract& override {
      if (route == child_name) {
        return child;
      }
      return Unknown::get_unknown();
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

  /// A staged Alias exposes only resolution outcomes while its graph owner
  /// keeps the one time binding operation behind the derived contract.
  class StagedAlias : public Alias {
   public:
    StagedAlias(View::Bytes name, const Documentation& documentation)
        : Alias(name, documentation) {}

    auto bind(const Abstract& target) -> Bool { return bind_target(target); }
  };

  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "Use the palette context."_view,
    "Preserve authored color names."_view,
  }};
  static constexpr Documentations::Comment graphics_comment(
      "The graphics context."_view);
  static constexpr Documentations::Block palette_comments(lines);
  static constexpr Documentations::Merged palette_documentation(
      palette_comments, graphics_comment);
  Value color("Color"_view);
  Context graphics("Graphics"_view, "Color"_view, color, graphics_comment);
  Alias palette("Palette"_view, graphics, palette_documentation);
  Alias colors("Colors"_view, palette);
  Alias deferred("Deferred"_view, color);
  StagedAlias staged("Staged"_view, palette_comments);
  Alias staged_nested("StagedNested"_view, staged);

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
  EXPECT(&palette.resolve_concept("Color"_view) == &None::get_none());
  EXPECT(&colors.resolve_concept("Color"_view) == &None::get_none());
  EXPECT(&colors.resolve().resolve_concept("Color"_view) == &color);
  EXPECT(&palette.resolve_concept("Missing"_view) == &None::get_none());
  EXPECT(&deferred.resolve() == &color);
  EXPECT(&color.resolve() == &Unknown::get_unknown());
  EXPECT(&staged.resolve() == &Unknown::get_unknown());
  EXPECT(&staged.resolve_concept("Member"_view) == &Unknown::get_unknown());
  EXPECT(&staged_nested.resolve() == &Unknown::get_unknown());

  EXPECT(staged.bind(graphics));
  EXPECT(staged.bind(graphics));
  EXPECT_NOT(staged.bind(color));

  EXPECT(&staged.resolve() == &graphics);
  EXPECT(&staged_nested.resolve() == &graphics);
  EXPECT(&staged.resolve_concept("Color"_view) == &None::get_none());
  EXPECT(&staged.resolve().resolve_concept("Color"_view) == &color);
}
