// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryOperation = {
  .name = "Tetrodotoxin::Library::Language::Operation"_view,
};

class OperationExpression : public Expression {
 public:
  OperationExpression(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  Ttx::Model::Layouts::Fluid inputs;
};

class TestOperation : public Operation {
 public:
  TestOperation(
      Allocator::Arena& domain,
      View::Bytes name,
      const Ttx::Model::Type& type,
      View::Vector<Reference<Expression>> inputs,
      const Expression& result,
      Bool fails = False)
      : Operation(domain, inputs),
        name(name),
        type(type),
        result(result),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&) const
      -> Result<const Expression&, FoldError> override {
    evaluations++;
    if (fails) {
      return FoldError::InvalidOperandType;
    }

    return result;
  }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  const Expression& result;
  Bool fails;
  mutable Count evaluations = 0;
};

static auto selects(
    const Result<const Expression&, FoldError>& result,
    const Expression& expected) -> Bool {
  return result.visit(
      [&](const Expression& selected) {
        return &selected == &expected ? True : False;
      },
      [](FoldError) { return False; });
}

static auto reports(
    const Result<const Expression&, FoldError>& result,
    FoldError expected) -> Bool {
  return result.visit(
      [](const Expression&) { return False; },
      [&](FoldError selected) { return selected == expected ? True : False; });
}

static auto input_is(
    const Operation& operation,
    Count index,
    const Expression& expected) -> Bool {
  return operation.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& selected) {
        return &selected == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryOperation, retains_nonconstant_inputs) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  Constants::Unsigned constant(type, 1);
  Static::Vector<Reference<Expression>, 2> inputs = {{ordinary, constant}};
  TestOperation operation(domain, "partial"_view, type, inputs, constant);

  auto first = operation.attempt_fold(domain, materializations);
  auto second = operation.attempt_fold(domain, materializations);

  EXPECT(selects(first, operation));
  EXPECT(selects(second, operation));
  EXPECT(input_is(operation, 0, ordinary));
  EXPECT(input_is(operation, 1, constant));
  EXPECT(operation.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, recursive_partial_fold) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  Constants::Unsigned input(type, 1);
  Constants::Unsigned folded(type, 2);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation child(domain, "child"_view, type, child_inputs, folded);
  Static::Vector<Reference<Expression>, 2> parent_inputs = {{child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, folded);

  auto first = parent.attempt_fold(domain, materializations);
  auto second = parent.attempt_fold(domain, materializations);

  EXPECT(selects(first, parent));
  EXPECT(selects(second, parent));
  EXPECT(input_is(parent, 0, folded));
  EXPECT(input_is(parent, 1, ordinary));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(parent.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, completed_result_is_idempotent) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_64 type;
  Constants::Unsigned first_input(type, 1);
  Constants::Unsigned second_input(type, 2);
  Constants::Unsigned folded(type, 3);
  Static::Vector<Reference<Expression>, 2> inputs = {{
    first_input,
    second_input,
  }};
  TestOperation operation(domain, "sum"_view, type, inputs, folded);

  auto first = operation.attempt_fold(domain, materializations);
  auto second = operation.attempt_fold(domain, materializations);

  EXPECT(selects(first, folded));
  EXPECT(selects(second, folded));
  EXPECT(input_is(operation, 0, first_input));
  EXPECT(input_is(operation, 1, second_input));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, child_failure_propagates) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  Constants::Unsigned input(type, 1);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation child(domain, "child"_view, type, child_inputs, input, True);
  Static::Vector<Reference<Expression>, 2> parent_inputs = {{child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, input);

  auto direct = child.attempt_fold(domain, materializations);
  auto folding = parent.attempt_fold(domain, materializations);

  EXPECT(reports(direct, FoldError::InvalidOperandType));
  EXPECT(reports(folding, FoldError::InvalidOperandType));
  EXPECT(input_is(parent, 0, child));
  EXPECT(input_is(parent, 1, ordinary));
  EXPECT(child.get_evaluations() == 2);
  EXPECT(parent.get_evaluations() == 0);
}
