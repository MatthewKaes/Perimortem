// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/context.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

using namespace Perimortem;
using namespace Ttx;
using namespace Validation;

class PackFact : public Concept::Abstract {
 public:
  constexpr explicit PackFact(Core::View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> Core::View::Bytes override { return name; }
  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }

 private:
  Core::View::Bytes name;
};

static Harness TtxPack = {
  .name = "Ttx::Concept::Pack"_view,
};

PERIMORTEM_UNIT_TEST(TtxPack, semantic_flow) {
  Memory::Allocator::Arena arena;
  PackFact left("Left"_view);
  PackFact right("Right"_view);
  const Core::Static::Vector<Concept::Reference<const Concept::Abstract>, 2>
      values = {{left, right}};
  const Core::Static::Vector<Core::View::Bytes, 2> names = {{
    "left"_view,
    "right"_view,
  }};
  Model::Layouts::Fluid values_layout(values);
  Model::Layouts::Named named(values_layout, names);
  Model::Context context(arena);

  const Concept::Pack& pack = context.pack(named);
  const Concept::Layout& layout = pack.get_layout();

  static_assert(!__is_base_of(Concept::Abstract, Concept::Pack));
  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(layout.get_abstract(0).visit(
      []() { return False; },
      [&](const Concept::Abstract& selected) {
        return &selected == &left ? True : False;
      }));
  EXPECT(layout.get_abstract(1).visit(
      []() { return False; },
      [&](const Concept::Abstract& selected) {
        return &selected == &right ? True : False;
      }));
  ASSERT(layout.get_name(0));
  ASSERT(layout.get_name(1));
  EXPECT_TEXT(*layout.get_name(0), "left"_view);
  EXPECT_TEXT(*layout.get_name(1), "right"_view);
}

PERIMORTEM_UNIT_TEST(TtxPack, empty_flow) {
  Memory::Allocator::Arena arena;
  Model::Layouts::Fluid empty;
  Model::Context context(arena);

  const Concept::Pack& pack = context.pack(empty);

  EXPECT(pack.get_layout().is_empty());
}
