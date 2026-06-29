// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"
#include "tetrodotoxin/ttx/graphics/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxGraphics = {
  .name = "Tetrodotoxin::Ttx::Graphics"_view,
};

static auto parse_type_ref(Allocator::Arena& arena, View::Bytes source)
    -> Syntax::Type::Ref {
  Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source, false);
  Syntax::Context context(tokenizer, "<inline type>"_view);
  context.set_color_enabled(False);
  return Syntax::Type::Ref::parse(context);
}

PERIMORTEM_UNIT_TEST(TtxGraphics, describes_graphics_types) {
  EXPECT(Ttx::Graphics::is_type("Sampler_2D"_view));
  EXPECT(Ttx::Graphics::is_type("Color"_view));
  EXPECT_NOT(Ttx::Graphics::is_type("Vec2D"_view));
  EXPECT(
      Ttx::Graphics::type_class("Sampler_2D"_view) ==
      Ttx::Type::Class::Sampler2D);
  EXPECT(Ttx::Graphics::type_class("Color"_view) == Ttx::Type::Class::Color);
}

PERIMORTEM_UNIT_TEST(TtxGraphics, describes_color_fields) {
  const Ttx::Type* color = Ttx::Graphics::find_type("Color"_view);
  EXPECT(color != nullptr);
  EXPECT(color->is_aggregate());
  EXPECT(color->get_layout() != nullptr);
  EXPECT(color->get_layout()->is_concrete());
  EXPECT_EQ(color->get_layout()->get_entries().get_size(), 4);
  EXPECT(Ttx::Graphics::type_class("Color"_view) == Ttx::Type::Class::Color);

  const Ttx::Field* red = Ttx::Graphics::find_field("Color"_view, "r"_view);
  EXPECT(red != nullptr);
  EXPECT_TEXT(red->get_type_name(), "Real_32"_view);
  EXPECT(Ttx::Graphics::find_field("Color"_view, "x"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxGraphics, describes_sampling_methods) {
  const Ttx::Type* sampler = Ttx::Graphics::find_type("Sampler_2D"_view);
  EXPECT(sampler != nullptr);
  EXPECT(sampler->get_layout() != nullptr);

  const Ttx::Method* sample =
      Ttx::Graphics::find_method("Sampler_2D"_view, "sample"_view);
  EXPECT(sample != nullptr);
  EXPECT(sampler->find_method("sample"_view) == sample);
  EXPECT(sample->get_args() != nullptr);
  EXPECT(sample->get_args()->is_fluid());
  EXPECT_EQ(sample->get_args()->get_field_count(), 1);
  EXPECT(sample->get_args()->field_at(0) != nullptr);
  EXPECT_TEXT(sample->get_args()->field_at(0)->get_type_name(), "Vec2D"_view);
  EXPECT_TEXT(sample->get_result_type_name(), "Color"_view);
  EXPECT(sample->get_result() != nullptr);
  EXPECT(sample->get_result()->is_aggregate());

  EXPECT(Ttx::Graphics::find_method("Color"_view, "sample"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxGraphics, returns_layout_facts_for_graphics_types) {
  Allocator::Arena arena;

  Ttx::Layout color =
      Ttx::Graphics::type_layout(parse_type_ref(arena, "Color"_view));
  EXPECT(color.is_valid());
  EXPECT(color.is_concrete());
  EXPECT_EQ(color.get_entries().get_size(), 4);
  EXPECT_EQ(color.get_byte_size(), 16);
  EXPECT_EQ(color.get_alignment(), 16);

  Ttx::Layout missing =
      Ttx::Graphics::type_layout(parse_type_ref(arena, "Missing"_view));
  EXPECT_NOT(missing.is_valid());
}
