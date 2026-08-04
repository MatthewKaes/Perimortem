// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/materializations.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryMaterializations = {
  .name = "Tetrodotoxin::Library::Language::Materializations"_view,
};

class MaterializedType : public Ttx::Model::Type {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Materialized"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class IncompleteType : public MaterializedType {
 public:
  auto resolve() const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class RedirectedType : public MaterializedType {
 public:
  constexpr RedirectedType(const Ttx::Model::Type& target) : target(target) {}

  constexpr auto resolve() const -> const Abstract& override { return target; }

 private:
  const Ttx::Model::Type& target;
};

class Formula : public Generic {
 public:
  constexpr Formula(View::Vector<Parameters> parameters)
      : parameters(parameters) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "Formula"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> View::Vector<Parameters> override {
    return parameters;
  }

 protected:
  View::Vector<Parameters> parameters;
};

class ProducingFormula : public Formula {
 public:
  constexpr ProducingFormula(View::Vector<Parameters> parameters)
      : Formula(parameters) {}

  constexpr auto get_constructions() const -> Count { return constructions; }

  auto create(View::Vector<Argument>, Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    return arena.construct<MaterializedType>();
  }

 private:
  mutable Count constructions = 0;
};

class RejectingFormula : public Formula {
 public:
  constexpr RejectingFormula(View::Vector<Parameters> parameters)
      : Formula(parameters) {}

  constexpr auto get_constructions() const -> Count { return constructions; }

  auto create(View::Vector<Argument>, Allocator::Arena&) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    return {};
  }

 private:
  mutable Count constructions = 0;
};

class ReturningFormula : public Formula {
 public:
  constexpr ReturningFormula(
      View::Vector<Parameters> parameters,
      const Ttx::Model::Type& result)
      : Formula(parameters), result(result) {}

  constexpr auto get_constructions() const -> Count { return constructions; }

  auto create(View::Vector<Argument>, Allocator::Arena&) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    return result;
  }

 private:
  const Ttx::Model::Type& result;
  mutable Count constructions = 0;
};

class RetryingFormula : public Formula {
 public:
  constexpr RetryingFormula() : Formula({}) {}

  constexpr auto get_constructions() const -> Count { return constructions; }

  auto create(View::Vector<Argument>, Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    if (constructions == 1) {
      return {};
    }

    return arena.construct<MaterializedType>();
  }

 private:
  mutable Count constructions = 0;
};

class RedirectedFormula : public ProducingFormula {
 public:
  constexpr RedirectedFormula(const Generic& target)
      : ProducingFormula({}), target(target) {}

  constexpr auto resolve() const -> const Abstract& override { return target; }

 private:
  const Generic& target;
};

class ChangingFormula : public ProducingFormula {
 public:
  constexpr ChangingFormula() : ProducingFormula(unsigned_parameterization) {}

  auto select_signed() -> void { parameters = signed_parameterization; }

 private:
  static constexpr Static::Vector<Parameters, 1> unsigned_parameterization = {
    {Parameters::Unsigned_64},
  };
  static constexpr Static::Vector<Parameters, 1> signed_parameterization = {
    {Parameters::Signed_64},
  };
};

class RecursiveFormula : public Formula {
 public:
  constexpr RecursiveFormula(Materializations& materializations)
      : Formula({}), materializations(materializations) {}

  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    materializations.materialize(*this, arguments);
    return arena.construct<MaterializedType>();
  }

  constexpr auto get_constructions() const -> Count { return constructions; }

 private:
  Materializations& materializations;
  mutable Count constructions = 0;
};

class IndirectFormula : public Formula {
 private:
  class Partner : public Formula {
   public:
    constexpr Partner(
        Materializations& materializations,
        const IndirectFormula& first)
        : Formula({}), materializations(materializations), first(first) {}

    auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
        -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
      constructions++;
      materializations.materialize(first, arguments);
      return arena.construct<MaterializedType>();
    }

    constexpr auto get_constructions() const -> Count { return constructions; }

   private:
    Materializations& materializations;
    const IndirectFormula& first;
    mutable Count constructions = 0;
  };

 public:
  constexpr IndirectFormula(Materializations& materializations)
      : Formula({}),
        materializations(materializations),
        partner(materializations, *this) {}

  auto create(View::Vector<Argument> arguments, Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override {
    constructions++;
    materializations.materialize(partner, arguments);
    return arena.construct<MaterializedType>();
  }

  constexpr auto get_constructions() const -> Count { return constructions; }
  constexpr auto get_partner_constructions() const -> Count {
    return partner.get_constructions();
  }

 private:
  Materializations& materializations;
  Partner partner;
  mutable Count constructions = 0;
};

static constexpr Static::Vector<Generic::Parameters, 1> type_parameters = {
  {Generic::Parameters::Type},
};
static constexpr Static::Vector<Generic::Parameters, 1> unsigned_parameters = {
  {Generic::Parameters::Unsigned_64},
};
static constexpr Static::Vector<Generic::Parameters, 2> signed_pair_parameters =
    {{
      Generic::Parameters::Signed_64,
      Generic::Parameters::Signed_64,
    }};
static constexpr Static::Vector<Generic::Parameters, 4> all_parameters = {{
  Generic::Parameters::Type,
  Generic::Parameters::Unsigned_64,
  Generic::Parameters::Signed_64,
  Generic::Parameters::Bool,
}};

PERIMORTEM_UNIT_TEST(LibraryMaterializations, concrete_formulas) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  Tetrodotoxin::Library::Language::Types::Unsigned_8 element;
  Generics::Access access;
  Generics::View view;
  Generics::Fixed fixed;
  const Static::Vector<Generic::Argument, 1> element_argument = {
    {Generic::Argument(element)},
  };
  const Static::Vector<Generic::Argument, 2> fixed_arguments = {{
    Generic::Argument(element),
    Generic::Argument(::Signed_64(4)),
  }};

  auto access_type = materializations.materialize(access, element_argument);
  auto view_type = materializations.materialize(view, element_argument);
  auto fixed_type = materializations.materialize(fixed, fixed_arguments);

  ASSERT(access_type);
  ASSERT(view_type);
  ASSERT(fixed_type);
  EXPECT(access_type->is<Types::Access>());
  EXPECT(view_type->is<Types::View>());
  EXPECT(fixed_type->is<Types::Fixed>());
  EXPECT(access_type->visit<Types::Access>(
      [&element](const Types::Access& selected) {
        return &selected.get_element_type() == &element ? True : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT(view_type->visit<Types::View>(
      [&element](const Types::View& selected) {
        return &selected.get_element_type() == &element ? True : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT(fixed_type->visit<Types::Fixed>(
      [&element](const Types::Fixed& selected) {
        return &selected.get_element_type() == &element &&
                       selected.get_extent() == ::Signed_64(4)
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT_EQ(materializations.get_size(), Count(3));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, exact_key) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula formula({});
  View::Vector<Generic::Argument> arguments;

  auto first = materializations.materialize(formula, arguments);
  auto repeated = materializations.materialize(formula, arguments);

  ASSERT(first);
  ASSERT(repeated);
  EXPECT(&*first == &*repeated);
  EXPECT_EQ(formula.get_constructions(), Count(1));
  EXPECT_EQ(materializations.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, formula_identity) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula first_formula({});
  ProducingFormula second_formula({});
  View::Vector<Generic::Argument> arguments;

  auto first = materializations.materialize(first_formula, arguments);
  auto second = materializations.materialize(second_formula, arguments);

  ASSERT(first);
  ASSERT(second);
  EXPECT(&*first != &*second);
  EXPECT_EQ(first_formula.get_constructions(), Count(1));
  EXPECT_EQ(second_formula.get_constructions(), Count(1));
  EXPECT_EQ(materializations.get_size(), Count(2));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, argument_identity) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula ordered(signed_pair_parameters);
  ProducingFormula scalar(unsigned_parameters);
  ProducingFormula typed(type_parameters);
  Tetrodotoxin::Library::Language::Types::Unsigned_8 first_type;
  Tetrodotoxin::Library::Language::Types::Unsigned_8 second_type;
  const Static::Vector<Generic::Argument, 2> first_order = {{
    Generic::Argument(::Signed_64(1)),
    Generic::Argument(::Signed_64(2)),
  }};
  const Static::Vector<Generic::Argument, 2> second_order = {{
    Generic::Argument(::Signed_64(2)),
    Generic::Argument(::Signed_64(1)),
  }};
  Static::Vector<Generic::Argument, 1> scalar_source = {
    {Generic::Argument(::Unsigned_64(7))},
  };
  const Static::Vector<Generic::Argument, 1> original_scalar = {
    {Generic::Argument(::Unsigned_64(7))},
  };
  const Generic::Argument second_scalar(::Unsigned_64(8));
  const Static::Vector<Generic::Argument, 1> first_edge = {
    {Generic::Argument(first_type)},
  };
  const Static::Vector<Generic::Argument, 1> second_edge = {
    {Generic::Argument(second_type)},
  };

  auto ordered_first = materializations.materialize(ordered, first_order);
  auto ordered_second = materializations.materialize(ordered, second_order);
  auto scalar_first = materializations.materialize(scalar, scalar_source);
  scalar_source[0] = second_scalar;
  auto scalar_second = materializations.materialize(scalar, scalar_source);
  auto scalar_repeated = materializations.materialize(scalar, original_scalar);
  auto typed_first = materializations.materialize(typed, first_edge);
  auto typed_second = materializations.materialize(typed, second_edge);

  ASSERT(ordered_first);
  ASSERT(ordered_second);
  ASSERT(scalar_first);
  ASSERT(scalar_second);
  ASSERT(scalar_repeated);
  ASSERT(typed_first);
  ASSERT(typed_second);
  EXPECT(&*ordered_first != &*ordered_second);
  EXPECT(&*scalar_first != &*scalar_second);
  EXPECT(&*scalar_first == &*scalar_repeated);
  EXPECT(&*typed_first != &*typed_second);
  EXPECT_EQ(materializations.get_size(), Count(6));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, alternative_identity) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ChangingFormula formula;
  const Static::Vector<Generic::Argument, 1> unsigned_argument = {
    {Generic::Argument(::Unsigned_64(7))},
  };
  const Static::Vector<Generic::Argument, 1> signed_argument = {
    {Generic::Argument(::Signed_64(7))},
  };

  auto unsigned_type = materializations.materialize(formula, unsigned_argument);
  formula.select_signed();
  auto signed_type = materializations.materialize(formula, signed_argument);

  ASSERT(unsigned_type);
  ASSERT(signed_type);
  EXPECT(&*unsigned_type != &*signed_type);
  EXPECT_EQ(formula.get_constructions(), Count(2));
  EXPECT_EQ(materializations.get_size(), Count(2));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, parameter_kinds) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula formula(all_parameters);
  Tetrodotoxin::Library::Language::Types::Unsigned_8 type;
  const Static::Vector<Generic::Argument, 4> arguments = {{
    Generic::Argument(type),
    Generic::Argument(::Unsigned_64(9)),
    Generic::Argument(::Signed_64(-3)),
    Generic::Argument(True),
  }};

  EXPECT(materializations.materialize(formula, arguments));
  EXPECT_EQ(formula.get_constructions(), Count(1));
  EXPECT_EQ(materializations.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, argument_rejection) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula formula(type_parameters);
  View::Vector<Generic::Argument> wrong_arity;
  const Static::Vector<Generic::Argument, 1> wrong_kind = {
    {Generic::Argument(::Unsigned_64(8))},
  };
  Static::Vector<Generic::Argument, 1> null_argument;

  EXPECT_NOT(materializations.materialize(formula, wrong_arity));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_NOT(materializations.materialize(formula, wrong_kind));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_NOT(materializations.materialize(formula, null_argument));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_EQ(formula.get_constructions(), Count(0));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, formula_resolution) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula target({});
  RedirectedFormula redirected(target);
  View::Vector<Generic::Argument> arguments;

  EXPECT_NOT(materializations.materialize(redirected, arguments));
  EXPECT_EQ(redirected.get_constructions(), Count(0));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, type_resolution) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  ProducingFormula formula(type_parameters);
  MaterializedType canonical;
  IncompleteType incomplete;
  RedirectedType redirected(canonical);
  const Static::Vector<Generic::Argument, 1> incomplete_argument = {
    {Generic::Argument(incomplete)},
  };
  const Static::Vector<Generic::Argument, 1> redirected_argument = {
    {Generic::Argument(redirected)},
  };

  EXPECT_NOT(materializations.materialize(formula, incomplete_argument));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_NOT(materializations.materialize(formula, redirected_argument));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_EQ(formula.get_constructions(), Count(0));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, result_rejection) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  RejectingFormula absent({});
  IncompleteType incomplete;
  MaterializedType canonical;
  RedirectedType redirected(canonical);
  ReturningFormula incomplete_result({}, incomplete);
  ReturningFormula redirected_result({}, redirected);
  View::Vector<Generic::Argument> arguments;

  EXPECT_NOT(materializations.materialize(absent, arguments));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_NOT(materializations.materialize(incomplete_result, arguments));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_NOT(materializations.materialize(redirected_result, arguments));
  EXPECT_EQ(materializations.get_size(), Count(0));
  EXPECT_EQ(absent.get_constructions(), Count(1));
  EXPECT_EQ(incomplete_result.get_constructions(), Count(1));
  EXPECT_EQ(redirected_result.get_constructions(), Count(1));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, retry) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  RetryingFormula formula;
  View::Vector<Generic::Argument> arguments;

  EXPECT_NOT(materializations.materialize(formula, arguments));
  EXPECT_EQ(materializations.get_size(), Count(0));
  auto successful = materializations.materialize(formula, arguments);
  auto repeated = materializations.materialize(formula, arguments);

  ASSERT(successful);
  ASSERT(repeated);
  EXPECT(&*successful == &*repeated);
  EXPECT_EQ(formula.get_constructions(), Count(2));
  EXPECT_EQ(materializations.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, direct_cycle) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  RecursiveFormula formula(materializations);
  View::Vector<Generic::Argument> arguments;

  EXPECT_NOT(materializations.materialize(formula, arguments));
  EXPECT_EQ(formula.get_constructions(), Count(1));
  EXPECT_EQ(materializations.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(LibraryMaterializations, indirect_cycle) {
  Allocator::Arena arena;
  Materializations materializations(arena);
  IndirectFormula formula(materializations);
  View::Vector<Generic::Argument> arguments;

  EXPECT_NOT(materializations.materialize(formula, arguments));
  EXPECT_EQ(formula.get_constructions(), Count(1));
  EXPECT_EQ(formula.get_partner_constructions(), Count(1));

  EXPECT_EQ(materializations.get_size(), Count(0));
}
