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
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

static Harness TtxGeneric = {
  .name = "TTX::Types::Generics"_view,
};

static_assert(sizeof(Types::Generic::Argument) <= 16);

class EquivalentNameType final : public Type {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Unsigned_8"_view;
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class ReservedType final : public Type {
 public:
  ReservedType(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }

  constexpr auto resolve() const -> const Abstract& override {
    return completed ? static_cast<const Abstract&>(*this)
                     : Invalid::get_invalid();
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

  constexpr auto complete() -> void { completed = True; }

 private:
  View::Bytes name;
  Bool completed = False;
};

class RedirectedType final : public Type {
 public:
  RedirectedType(const Type& target) : target(target) {}

  constexpr auto get_name() const -> View::Bytes override {
    return target.get_name();
  }

  constexpr auto resolve() const -> const Abstract& override {
    return target.resolve();
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return target.get_documentation();
  }

  constexpr auto resolve_context(View::Bytes route) const
      -> const Abstract& override {
    return target.resolve_context(route);
  }

 private:
  const Type& target;
};

class ResultGeneric final : public Types::Generic {
 public:
  ResultGeneric(const Type& result) : result(result) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "Result"_view;
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(View::Vector<Argument>, Allocator::Arena&) const
      -> Option<const Type&> override {
    return result;
  }

  const Type& result;
  static constexpr Static::Vector<Parameters, 1> parameterization = {{
    Parameters::Type,
  }};
};

class ReentrantGeneric final : public Types::Generic {
 public:
  ReentrantGeneric(Materializations& materializations)
      : materializations(materializations) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "Reentrant"_view;
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Option<const Type&> override {
    materializations.materialize(*this, arguments);
    return arena.construct<EquivalentNameType>();
  }

  Materializations& materializations;
  static constexpr Static::Vector<Parameters, 1> parameterization = {{
    Parameters::Type,
  }};
};

class CyclingGeneric final : public Types::Generic {
 public:
  CyclingGeneric(Materializations& materializations)
      : materializations(materializations) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "Cycling"_view;
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Option<const Type&> override {
    Signed_64 value = *arguments[0].find<Signed_64>();
    const Static::Vector<Argument, 1> nested = {
      {Signed_64(value == 0 ? 1 : 0)}};
    materializations.materialize(*this, nested);
    return arena.construct<EquivalentNameType>();
  }

  Materializations& materializations;
  static constexpr Static::Vector<Parameters, 1> parameterization = {{
    Parameters::Signed_64,
  }};
};

static auto is_none(const Option<const Type&>& selected) -> Bool {
  return selected.visit(
      [](const None&) { return True; }, [](const Type&) { return False; });
}

static auto get_type(const Option<const Type&>& selected) -> const Type& {
  return selected.visit(
      [](const None&) -> const Type& { __builtin_unreachable(); },
      [](const Type& type) -> const Type& { return type; });
}

static auto materialize_view(
    Types::Generic::Materializations& materializations,
    const Types::Generics::View& view,
    const Type& element) -> const Type& {
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};
  return get_type(materializations.materialize(view, arguments));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, publishes_parameterization) {
  Allocator::Arena arena;
  Types::Generics::View view;
  Types::Generics::Access access;
  Types::Generics::Fixed fixed;
  auto view_parameters = view.get_parameterization();
  auto access_parameters = access.get_parameterization();
  auto fixed_parameters = fixed.get_parameterization();

  EXPECT(view.is<Types::Generic>());
  EXPECT_NOT(view.is<Type>());
  EXPECT_EQ(view_parameters.get_size(), Count(1));
  EXPECT_EQ(access_parameters.get_size(), Count(1));
  EXPECT_EQ(fixed_parameters.get_size(), Count(2));
  EXPECT(view_parameters[0] == Types::Generic::Parameters::Type);
  EXPECT(access_parameters[0] == Types::Generic::Parameters::Type);
  EXPECT(fixed_parameters[0] == Types::Generic::Parameters::Type);
  EXPECT(fixed_parameters[1] == Types::Generic::Parameters::Signed_64);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materializes_class_specific_types) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  Types::Unsigned_8 element;
  Types::Generics::View view;
  Types::Generics::Access access;
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};

  Option<const Type&> first_view =
      materializations.materialize(view, arguments);
  Option<const Type&> second_view =
      materializations.materialize(view, arguments);
  Option<const Type&> first_access =
      materializations.materialize(access, arguments);
  const Type& view_type = get_type(first_view);
  const Type& access_type = get_type(first_access);

  EXPECT(&view_type == &get_type(second_view));
  EXPECT(view_type.is<Types::Generics::View::Type>());
  EXPECT_NOT(view_type.is<Types::Generics::Access::Type>());
  EXPECT(access_type.is<Types::Generics::Access::Type>());
  EXPECT_NOT(access_type.is<Types::Generics::View::Type>());
  EXPECT_TEXT(view_type.get_name(), "View[Unsigned_8]"_view);
  EXPECT_TEXT(access_type.get_name(), "Access[Unsigned_8]"_view);

  const auto& concrete_view = view_type.assume<Types::Generics::View::Type>();
  const auto& concrete_access =
      access_type.assume<Types::Generics::Access::Type>();
  EXPECT(&concrete_view.get_element_type() == &element);
  EXPECT(&concrete_access.get_element_type() == &element);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materializes_fixed_ranged_layouts) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  Types::Unsigned_8 element;
  EquivalentNameType equivalent_name;
  Types::Generics::Fixed fixed;
  const Static::Vector<Types::Generic::Argument, 2> four_arguments = {{
    element,
    Signed_64(4),
  }};
  const Static::Vector<Types::Generic::Argument, 2> six_arguments = {{
    element,
    Signed_64(6),
  }};
  const Static::Vector<Types::Generic::Argument, 2> equivalent_arguments = {{
    equivalent_name,
    Signed_64(4),
  }};

  const Type& first =
      get_type(materializations.materialize(fixed, four_arguments));
  const Type& repeated =
      get_type(materializations.materialize(fixed, four_arguments));
  const Type& distinct_extent =
      get_type(materializations.materialize(fixed, six_arguments));
  const Type& distinct_element =
      get_type(materializations.materialize(fixed, equivalent_arguments));
  const auto& concrete = first.assume<Types::Generics::Fixed::Type>();
  const Layout& layout = concrete.get_layout();

  EXPECT(&first == &repeated);
  EXPECT(&first != &distinct_extent);
  EXPECT(&first != &distinct_element);
  EXPECT_TEXT(first.get_name(), distinct_element.get_name());
  EXPECT(first.is<Types::Generics::Fixed::Type>());
  EXPECT_TEXT(first.get_name(), "Fixed[Unsigned_8,4]"_view);
  EXPECT(&concrete.get_element_type() == &element);
  EXPECT_EQ(concrete.get_extent(), Signed_64(4));
  EXPECT_EQ(layout.get_size(), Count(4));
  EXPECT(&layout.get_abstract(0) == &element);
  EXPECT(&layout.get_abstract(3) == &element);
  EXPECT(layout.get_abstract(4).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxGeneric, materialization_uses_semantic_identity) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  Types::Unsigned_8 element;
  EquivalentNameType equivalent_name;
  Types::Generics::View view;
  const Static::Vector<Types::Generic::Argument, 1> element_arguments = {
    {element}};
  const Static::Vector<Types::Generic::Argument, 1> equivalent_arguments = {
    {equivalent_name}};

  const Type& concrete_element =
      get_type(materializations.materialize(view, element_arguments));
  const Type& concrete_equivalent =
      get_type(materializations.materialize(view, equivalent_arguments));

  EXPECT(&concrete_element != &concrete_equivalent);
  EXPECT_TEXT(concrete_element.get_name(), concrete_equivalent.get_name());
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejects_wrong_argument_shapes) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  Types::Unsigned_8 element;
  Types::Generics::View view;
  const Static::Vector<Types::Generic::Argument, 1> unsigned_argument = {
    {Unsigned_64(8)}};
  const Static::Vector<Types::Generic::Argument, 1> bool_argument = {{True}};
  const Static::Vector<Types::Generic::Argument, 2> extra_arguments = {
    {element, element}};

  EXPECT(is_none(materializations.materialize(view, {})));
  EXPECT(is_none(materializations.materialize(view, unsigned_argument)));
  EXPECT(is_none(materializations.materialize(view, bool_argument)));
  EXPECT(is_none(materializations.materialize(view, extra_arguments)));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, fixed_rejects_non_ranges) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  Types::Unsigned_8 element;
  Types::Generics::Fixed fixed;
  const Static::Vector<Types::Generic::Argument, 2> negative = {{
    element,
    Signed_64(-1),
  }};
  const Static::Vector<Types::Generic::Argument, 2> unsigned_extent = {{
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
  Types::Generic::Materializations first_materializations(first_arena);
  Types::Generic::Materializations second_materializations(second_arena);
  Types::Unsigned_8 element;
  Types::Generics::View view;
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};

  const Type& first_type =
      get_type(first_materializations.materialize(view, arguments));
  const Type& second_type =
      get_type(second_materializations.materialize(view, arguments));

  EXPECT(&first_type != &second_type);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, retained_keys_follow_owner_lifetimes) {
  Allocator::Arena arena;
  Types::Unsigned_8 element;
  Types::Generics::View view;
  Types::Generic::Materializations materializations(arena);

  const Type& first = materialize_view(materializations, view, element);
  const Type& repeated = materialize_view(materializations, view, element);

  EXPECT(&first == &repeated);
  EXPECT_EQ(materializations.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejected_keys_can_complete_progressively) {
  Allocator::Arena arena;
  Types::Generics::View view;
  ReservedType element("Deferred"_view);
  Types::Generic::Materializations materializations(arena);
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};

  Option<const Type&> rejected = materializations.materialize(view, arguments);
  EXPECT_EQ(materializations.get_size(), Count(0));

  element.complete();
  Option<const Type&> completed = materializations.materialize(view, arguments);
  Option<const Type&> repeated = materializations.materialize(view, arguments);

  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(1));
  EXPECT(&get_type(completed) == &get_type(repeated));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejects_redirected_type_arguments) {
  Allocator::Arena arena;
  Types::Unsigned_8 element;
  RedirectedType redirected(element);
  Types::Generics::View view;
  Types::Generic::Materializations materializations(arena);
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{redirected}};

  Option<const Type&> rejected = materializations.materialize(view, arguments);

  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, incomplete_results_publish_no_key) {
  Allocator::Arena arena;
  Types::Unsigned_8 element;
  ReservedType deferred_result("DeferredResult"_view);
  ResultGeneric generic(deferred_result);
  Types::Generic::Materializations materializations(arena);
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};

  Option<const Type&> rejected =
      materializations.materialize(generic, arguments);
  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(0));

  deferred_result.complete();
  Option<const Type&> completed =
      materializations.materialize(generic, arguments);

  EXPECT(&get_type(completed) == &deferred_result);
  EXPECT_EQ(materializations.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, same_key_reentrancy_is_rejected) {
  Allocator::Arena arena;
  Types::Unsigned_8 element;
  Types::Generic::Materializations materializations(arena);
  ReentrantGeneric generic(materializations);
  const Static::Vector<Types::Generic::Argument, 1> arguments = {{element}};

  Option<const Type&> rejected =
      materializations.materialize(generic, arguments);

  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, reentrant_key_cycles_are_rejected) {
  Allocator::Arena arena;
  Types::Generic::Materializations materializations(arena);
  CyclingGeneric generic(materializations);
  const Static::Vector<Types::Generic::Argument, 1> arguments = {
    {Signed_64(0)}};

  Option<const Type&> rejected =
      materializations.materialize(generic, arguments);

  EXPECT(is_none(rejected));
  EXPECT_EQ(materializations.get_size(), Count(0));
}
