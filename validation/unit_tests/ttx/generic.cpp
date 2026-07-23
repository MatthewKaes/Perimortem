// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/generics/access.hpp"
#include "ttx/model/types/generics/fixed.hpp"
#include "ttx/model/types/generics/view.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx;
using namespace Validation;

static Harness TtxGeneric = {
  .name = "TTX::Types::Generics"_view,
};

static_assert(sizeof(Model::Types::Generic::Argument) <= 16);

class EquivalentNameType final : public Model::Type {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Unsigned_8"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }
};

class DeferredGeneric final : public Model::Types::Generic {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Deferred"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return Concept::Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameterization;
  }

  constexpr auto complete() -> void { ready = True; }

 private:
  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Option<const Model::Type&> override {
    if (!ready || arguments.get_size() != 1 ||
        arguments[0].find<const Model::Type&>() == nullptr) {
      return none;
    }

    return arena.construct<EquivalentNameType>();
  }

  Bool ready = False;
  static constexpr Static::Vector<Parameters, 1> parameterization = {{
    Parameters::Type,
  }};
};

static auto is_none(const Option<const Model::Type&>& selected) -> Bool {
  return selected.visit(
      [](const None&) { return True; },
      [](const Model::Type&) { return False; });
}

static auto get_type(const Option<const Model::Type&>& selected)
    -> const Model::Type& {
  return selected.visit(
      [](const None&) -> const Model::Type& { __builtin_unreachable(); },
      [](const Model::Type& type) -> const Model::Type& { return type; });
}

PERIMORTEM_UNIT_TEST(TtxGeneric, publishes_parameterization) {
  Allocator::Arena arena;
  Model::Types::Generics::View view;
  Model::Types::Generics::Access access;
  Model::Types::Generics::Fixed fixed;
  auto view_parameters = view.get_parameterization();
  auto access_parameters = access.get_parameterization();
  auto fixed_parameters = fixed.get_parameterization();

  EXPECT(view.is<Model::Types::Generic>());
  EXPECT_NOT(view.is<Model::Type>());
  EXPECT_EQ(view_parameters.get_size(), Count(1));
  EXPECT_EQ(access_parameters.get_size(), Count(1));
  EXPECT_EQ(fixed_parameters.get_size(), Count(2));
  EXPECT(view_parameters[0] == Model::Types::Generic::Parameters::Type);
  EXPECT(access_parameters[0] == Model::Types::Generic::Parameters::Type);
  EXPECT(fixed_parameters[0] == Model::Types::Generic::Parameters::Type);
  EXPECT(fixed_parameters[1] == Model::Types::Generic::Parameters::Signed_64);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materializes_class_specific_types) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View view;
  Model::Types::Generics::Access access;
  const Static::Vector<Model::Types::Generic::Argument, 1> arguments = {
    {element}};

  Option<const Model::Type&> first_view =
      materializations.materialize(view, arguments);
  Option<const Model::Type&> second_view =
      materializations.materialize(view, arguments);
  Option<const Model::Type&> first_access =
      materializations.materialize(access, arguments);
  const Model::Type& view_type = get_type(first_view);
  const Model::Type& access_type = get_type(first_access);

  EXPECT(&view_type == &get_type(second_view));
  EXPECT(view_type.is<Model::Types::Generics::View::Type>());
  EXPECT_NOT(view_type.is<Model::Types::Generics::Access::Type>());
  EXPECT(access_type.is<Model::Types::Generics::Access::Type>());
  EXPECT_NOT(access_type.is<Model::Types::Generics::View::Type>());
  EXPECT_TEXT(view_type.get_name(), "View[Unsigned_8]"_view);
  EXPECT_TEXT(access_type.get_name(), "Access[Unsigned_8]"_view);

  const auto& concrete_view =
      view_type.assume<Model::Types::Generics::View::Type>();
  const auto& concrete_access =
      access_type.assume<Model::Types::Generics::Access::Type>();
  EXPECT(&concrete_view.get_element_type() == &element);
  EXPECT(&concrete_access.get_element_type() == &element);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materializes_fixed_ranged_layouts) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  EquivalentNameType equivalent_name;
  Model::Types::Generics::Fixed fixed;
  const Static::Vector<Model::Types::Generic::Argument, 2> four_arguments = {{
    element,
    Signed_64(4),
  }};
  const Static::Vector<Model::Types::Generic::Argument, 2> six_arguments = {{
    element,
    Signed_64(6),
  }};
  const Static::Vector<Model::Types::Generic::Argument, 2>
      equivalent_arguments = {{
        equivalent_name,
        Signed_64(4),
      }};

  const Model::Type& first =
      get_type(materializations.materialize(fixed, four_arguments));
  const Model::Type& repeated =
      get_type(materializations.materialize(fixed, four_arguments));
  const Model::Type& distinct_extent =
      get_type(materializations.materialize(fixed, six_arguments));
  const Model::Type& distinct_element =
      get_type(materializations.materialize(fixed, equivalent_arguments));
  const auto& concrete = first.assume<Model::Types::Generics::Fixed::Type>();
  const Concept::Layout& layout = concrete.get_layout();

  EXPECT(&first == &repeated);
  EXPECT(&first != &distinct_extent);
  EXPECT(&first != &distinct_element);
  EXPECT_TEXT(first.get_name(), distinct_element.get_name());
  EXPECT(first.is<Model::Types::Generics::Fixed::Type>());
  EXPECT_TEXT(first.get_name(), "Fixed[Unsigned_8,4]"_view);
  EXPECT(&concrete.get_element_type() == &element);
  EXPECT_EQ(concrete.get_extent(), Signed_64(4));
  EXPECT_EQ(layout.get_size(), Count(4));
  EXPECT(&layout.get_abstract(0) == &element);
  EXPECT(&layout.get_abstract(3) == &element);
  EXPECT(layout.get_abstract(4).is<Concept::Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materialization_uses_semantic_identity) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  EquivalentNameType equivalent_name;
  Model::Types::Generics::View view;
  const Static::Vector<Model::Types::Generic::Argument, 1> element_arguments = {
    {element}};
  const Static::Vector<Model::Types::Generic::Argument, 1>
      equivalent_arguments = {{equivalent_name}};

  const Model::Type& concrete_element =
      get_type(materializations.materialize(view, element_arguments));
  const Model::Type& concrete_equivalent =
      get_type(materializations.materialize(view, equivalent_arguments));

  EXPECT(&concrete_element != &concrete_equivalent);
  EXPECT_TEXT(concrete_element.get_name(), concrete_equivalent.get_name());
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejects_wrong_argument_shapes) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View view;
  const Static::Vector<Model::Types::Generic::Argument, 1> unsigned_argument = {
    {Unsigned_64(8)}};
  const Static::Vector<Model::Types::Generic::Argument, 1> bool_argument = {
    {True}};
  const Static::Vector<Model::Types::Generic::Argument, 2> extra_arguments = {
    {element, element}};

  EXPECT(is_none(materializations.materialize(view, {})));
  EXPECT(is_none(materializations.materialize(view, unsigned_argument)));
  EXPECT(is_none(materializations.materialize(view, bool_argument)));
  EXPECT(is_none(materializations.materialize(view, extra_arguments)));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, fixed_rejects_non_ranges) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::Fixed fixed;
  const Static::Vector<Model::Types::Generic::Argument, 2> negative = {{
    element,
    Signed_64(-1),
  }};
  const Static::Vector<Model::Types::Generic::Argument, 2> unsigned_extent = {{
    element,
    Unsigned_64(4),
  }};

  EXPECT(is_none(materializations.materialize(fixed, negative)));
  EXPECT(is_none(materializations.materialize(fixed, unsigned_extent)));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materializations_are_transaction_local) {
  Allocator::Arena first_arena;
  Allocator::Arena second_arena;
  Model::Types::Generic::Materializations first_materializations(first_arena);
  Model::Types::Generic::Materializations second_materializations(second_arena);
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View view;
  const Static::Vector<Model::Types::Generic::Argument, 1> arguments = {
    {element}};

  const Model::Type& first_type =
      get_type(first_materializations.materialize(view, arguments));
  const Model::Type& second_type =
      get_type(second_materializations.materialize(view, arguments));

  EXPECT(&first_type != &second_type);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejected_keys_can_complete_progressively) {
  Allocator::Arena arena;
  Model::Types::Generic::Materializations materializations(arena);
  Model::Types::Unsigned_8 element;
  DeferredGeneric generic;
  const Static::Vector<Model::Types::Generic::Argument, 1> arguments = {
    {element}};

  Option<const Model::Type&> rejected =
      materializations.materialize(generic, arguments);
  generic.complete();
  Option<const Model::Type&> completed =
      materializations.materialize(generic, arguments);
  Option<const Model::Type&> repeated =
      materializations.materialize(generic, arguments);

  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(1));
  EXPECT(&get_type(completed) == &get_type(repeated));
}
