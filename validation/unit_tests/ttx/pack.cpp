// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/pack.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

// PackType supplies one exact terminal Layout entry so Pack fitting observes a
// real Type identity rather than a test only scalar surrogate.
class PackType : public Type {
 public:
  constexpr explicit PackType(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

// FlowPack models the only lifecycle distinction owned by the shared Pack
// contract. Its output Layout already exists at a stable address, but consumers
// may observe it only after the Pack resolves successfully.
class FlowPack : public Pack {
 public:
  constexpr FlowPack(
      View::Bytes name,
      const Layout& layout,
      Bool complete = True)
      : name(name), layout(layout), complete(complete) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  constexpr auto resolve() const -> const Abstract& override {
    return complete ? static_cast<const Abstract&>(*this)
                    : static_cast<const Abstract&>(Invalid::get_invalid());
  }
  constexpr auto get_layout() const -> const Layout& override { return layout; }

  constexpr auto get_produced(Count index) const
      -> Option<Pack::Produced> override {
    if (!complete || index >= layout.get_size()) {
      return {};
    }
    return Pack::Produced{*this, index};
  }

  constexpr auto complete_output() -> void { complete = True; }

 private:
  View::Bytes name;
  const Layout& layout;
  Bool complete;
};

static Harness TtxPack = {
  .name = "Ttx::Model::Pack"_view,
};

PERIMORTEM_UNIT_TEST(TtxPack, output_contract) {
  PackType left("Left"_view);
  PackType right("Right"_view);
  const Static::Vector<Reference<const Abstract>, 2> values = {{left, right}};
  Fluid output(values);
  FlowPack pack("Pair"_view, output);

  EXPECT(pack.is<Pack>());
  EXPECT_NOT(pack.is<Type>());
  EXPECT(&pack.resolve() == &pack);
  EXPECT(&pack.get_layout() == &output);
  EXPECT_EQ(pack.get_layout().get_size(), Count(2));
  EXPECT(pack.fits(output));
  auto produced = pack.get_produced(1);
  ASSERT(produced);
  EXPECT(&produced->producer == &pack);
  EXPECT_EQ(produced->local_index, Count(1));
  EXPECT_NOT(pack.get_produced(2));

  PackType value("Value"_view);
  const Static::Vector<Reference<const Abstract>, 1> staged_values = {{value}};
  Fluid staged_output(staged_values);
  FlowPack staged("Staged"_view, staged_output, False);

  EXPECT(staged.resolve().is<Invalid>());
  EXPECT_NOT(staged.get_produced(0));

  staged.complete_output();

  EXPECT(&staged.resolve() == &staged);
  EXPECT(&staged.get_layout() == &staged_output);
  auto staged_value = staged.get_produced(0);
  ASSERT(staged_value);
  EXPECT(&staged_value->producer == &staged);
}

PERIMORTEM_UNIT_TEST(TtxPack, empty_flow) {
  Fluid output;
  Fluid required;
  FlowPack pack("Empty"_view, output);

  EXPECT(pack.get_layout().is_empty());
  EXPECT(pack.fits(required));
  EXPECT_NOT(pack.is<Type>());
}
