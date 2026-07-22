// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/generics/access.hpp"
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

static auto is_none(const Option<Model::Type&>& selected) -> Bool {
  return selected.visit(
      [](const None&) { return True; },
      [](const Model::Type&) { return False; });
}

static auto get_type(const Option<Model::Type&>& selected) -> Model::Type& {
  return selected.visit(
      [](const None&) -> Model::Type& { __builtin_unreachable(); },
      [](Model::Type& type) -> Model::Type& { return type; });
}

PERIMORTEM_UNIT_TEST(TtxGeneric, publishes_parameterization) {
  Allocator::Arena arena;
  Model::Types::Generics::View view(arena);
  Model::Types::Generics::Access access(arena);
  auto view_parameters = view.get_parameterization();
  auto access_parameters = access.get_parameterization();

  EXPECT(view.is<Model::Types::Generic>());
  EXPECT_NOT(view.is<Model::Type>());
  EXPECT_EQ(view_parameters.get_size(), Count(1));
  EXPECT_EQ(access_parameters.get_size(), Count(1));
  EXPECT(view_parameters[0] == Model::Types::Generic::Parameters::Type);
  EXPECT(access_parameters[0] == Model::Types::Generic::Parameters::Type);
}

PERIMORTEM_UNIT_TEST(TtxGeneric, caches_class_specific_types) {
  Allocator::Arena arena;
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View view(arena);
  Model::Types::Generics::Access access(arena);
  const Static::Vector<Model::Types::Generic::Argument, 1> arguments = {
    {element}};

  Option<Model::Type&> first_view = view.find(arguments);
  Option<Model::Type&> second_view = view.find(arguments);
  Option<Model::Type&> first_access = access.find(arguments);
  Model::Type& view_type = get_type(first_view);
  Model::Type& access_type = get_type(first_access);

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

PERIMORTEM_UNIT_TEST(TtxGeneric, cache_uses_semantic_identity) {
  Allocator::Arena arena;
  Model::Types::Unsigned_8 element;
  EquivalentNameType equivalent_name;
  Model::Types::Generics::View view(arena);
  const Static::Vector<Model::Types::Generic::Argument, 1> element_arguments = {
    {element}};
  const Static::Vector<Model::Types::Generic::Argument, 1>
      equivalent_arguments = {{equivalent_name}};

  Model::Type& concrete_element = get_type(view.find(element_arguments));
  Model::Type& concrete_equivalent = get_type(view.find(equivalent_arguments));

  EXPECT(&concrete_element != &concrete_equivalent);
  EXPECT_TEXT(concrete_element.get_name(), concrete_equivalent.get_name());
}

PERIMORTEM_UNIT_TEST(TtxGeneric, rejects_wrong_argument_shapes) {
  Allocator::Arena arena;
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View view(arena);
  const Static::Vector<Model::Types::Generic::Argument, 1> unsigned_argument = {
    {Unsigned_64(8)}};
  const Static::Vector<Model::Types::Generic::Argument, 1> bool_argument = {
    {True}};
  const Static::Vector<Model::Types::Generic::Argument, 2> extra_arguments = {
    {element, element}};

  EXPECT(is_none(view.find({})));
  EXPECT(is_none(view.find(unsigned_argument)));
  EXPECT(is_none(view.find(bool_argument)));
  EXPECT(is_none(view.find(extra_arguments)));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, caches_are_formula_local) {
  Allocator::Arena first_arena;
  Allocator::Arena second_arena;
  Model::Types::Unsigned_8 element;
  Model::Types::Generics::View first(first_arena);
  Model::Types::Generics::View second(second_arena);
  const Static::Vector<Model::Types::Generic::Argument, 1> arguments = {
    {element}};

  Model::Type& first_type = get_type(first.find(arguments));
  Model::Type& second_type = get_type(second.find(arguments));

  EXPECT(&first_type != &second_type);
}
