// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryOperation = {
  .name = "Tetrodotoxin::Library::Language::Operation"_view,
};

class OperationMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  OperationMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "OperationMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class OperationExpression : public Expression {
 public:
  OperationExpression(View::Bytes name, const Ttx::Model::Type& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_finalizations() const -> Count { return finalizations; }

  auto finalize() -> void override {
    finalizations++;
    Expression::finalize();
  }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  Count finalizations = 0;
};

class TestOperation : public Operation {
 public:
  TestOperation(
      Allocator::Arena& domain,
      View::Bytes name,
      const Ttx::Model::Type& type,
      View::Vector<Reference<Expression>> inputs,
      Constant& result,
      Bool fails = False,
      Bool skips_after_first = False)
      : Operation(domain, inputs, {}),
        name(name),
        type(type),
        result(result),
        fails(fails),
        skips_after_first(skips_after_first) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }
  auto input_is(Count index, const Expression& expected) const -> Bool {
    return get_input(index).visit(
        []() { return False; },
        [&](const Expression& selected) {
          return &selected == &expected ? True : False;
        });
  }
  auto input_missing(Count index) const -> Bool { return !get_input(index); }

 protected:
  auto evaluate_constants(Allocator::Arena&)
      -> Result<Option<Constant&>, Expression::Error> override {
    evaluations++;
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(Tetrodotoxin::Language::Monograph&) const
      -> Option<const Ttx::Model::Type&> override {
    return type;
  }

  auto reaches_next_input(Count folded_input, const Expression&) const
      -> Bool override {
    return !skips_after_first || folded_input != 0;
  }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  Constant& result;
  Bool fails;
  Bool skips_after_first;
  Count evaluations = 0;
};

static auto selects(
    const Result<Option<Expression&>, Expression::Error>& result,
    const Expression& expected) -> Bool {
  return result.visit(
      [&](const Option<Expression&>& selected) {
        return selected && &*selected == &expected ? True : False;
      },
      [](const Expression::Error&) { return False; });
}

static auto reports(
    const Result<Option<Expression&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Expression& expression) -> Bool {
  return result.visit(
      [](const Option<Expression&>&) { return False; },
      [&](const Expression::Error& selected) {
        return selected.get_type() == expected &&
                       &selected.get_expression() == &expression
                   ? True
                   : False;
      });
}

static auto is_dynamic(
    const Result<Option<Expression&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Expression&>& selected) {
        return !selected ? True : False;
      },
      [](const Expression::Error&) { return False; });
}

PERIMORTEM_UNIT_TEST(LibraryOperation, retains_nonconstant_inputs) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& constant = Constants::Unsigned::create_synthetic(domain, type, 1);
  Static::Vector<Reference<Expression>, 2> inputs = {{ordinary, constant}};
  TestOperation operation(domain, "partial"_view, type, inputs, constant);

  EXPECT(operation.get_type().resolve().is<Invalid>());
  EXPECT(operation.link(source, Invalid::get_invalid()));
  EXPECT(operation.link(source, Invalid::get_invalid()));
  EXPECT(&operation.get_type() == &type);

  auto first = operation.fold();
  auto second = operation.fold();

  EXPECT(is_dynamic(first));
  EXPECT(is_dynamic(second));
  EXPECT(operation.input_is(0, ordinary));
  EXPECT(operation.input_is(1, constant));
  EXPECT(operation.input_missing(2));
  EXPECT(operation.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, finalizes_canonical_inputs) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression first("first"_view, type);
  OperationExpression second("second"_view, type);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 3);
  Static::Vector<Reference<Expression>, 2> inputs = {{first, second}};
  TestOperation operation(domain, "finalized"_view, type, inputs, folded);

  ASSERT(operation.link(source, Invalid::get_invalid()));
  operation.finalize();

  EXPECT(first.get_finalizations() == 1);
  EXPECT(second.get_finalizations() == 1);
  EXPECT(operation.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, recursive_partial_fold) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation child(domain, "child"_view, type, child_inputs, folded);
  Static::Vector<Reference<Expression>, 2> parent_inputs = {{child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, folded);

  EXPECT(parent.get_type().resolve().is<Invalid>());
  EXPECT(parent.link(source, Invalid::get_invalid()));
  EXPECT(&parent.get_type() == &type);

  auto first = parent.fold();
  auto second = parent.fold();

  EXPECT(is_dynamic(first));
  EXPECT(is_dynamic(second));
  EXPECT(selects(child.fold(), folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(parent.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, completed_result_is_idempotent) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  auto& first_input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& second_input = Constants::Unsigned::create_synthetic(domain, type, 2);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 3);
  Static::Vector<Reference<Expression>, 2> inputs = {{
    first_input,
    second_input,
  }};
  TestOperation operation(domain, "sum"_view, type, inputs, folded);

  EXPECT(operation.get_type().resolve().is<Invalid>());
  EXPECT(operation.link(source, Invalid::get_invalid()));
  EXPECT(&operation.get_type() == &type);

  auto first = operation.fold();
  auto second = operation.fold();

  EXPECT(selects(first, folded));
  EXPECT(selects(second, folded));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, child_failure_propagates) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation child(domain, "child"_view, type, child_inputs, input, True);
  Static::Vector<Reference<Expression>, 2> parent_inputs = {{child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, input);

  EXPECT(parent.get_type().resolve().is<Invalid>());
  EXPECT(parent.link(source, Invalid::get_invalid()));
  EXPECT(&parent.get_type() == &type);

  auto direct = child.fold();
  auto folding = parent.fold();
  auto repeated = parent.fold();

  EXPECT(reports(direct, Expression::Error::Type::InvalidConstant, child));
  EXPECT(reports(folding, Expression::Error::Type::InvalidConstant, child));
  EXPECT(reports(repeated, Expression::Error::Type::InvalidConstant, child));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(parent.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, changed_result_type_rejects) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 expected_type;
  Types::Unsigned_8 changed_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, expected_type, 1);
  auto& changed =
      Constants::Unsigned::create_synthetic(domain, changed_type, 1);
  Static::Vector<Reference<Expression>, 1> inputs = {{input}};
  TestOperation operation(
      domain, "changed"_view, expected_type, inputs, changed);

  EXPECT(operation.get_type().resolve().is<Invalid>());
  EXPECT(operation.link(source, Invalid::get_invalid()));
  EXPECT(&operation.get_type() == &expected_type);

  auto folded = operation.fold();

  EXPECT(
      reports(folded, Expression::Error::Type::ResultTypeMismatch, operation));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, prelink_query_remains_retryable) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Reference<Expression>, 1> inputs = {{input}};
  TestOperation operation(domain, "retry"_view, type, inputs, folded);

  auto before = operation.fold();
  EXPECT(is_dynamic(before));
  EXPECT(operation.get_evaluations() == 0);

  ASSERT(operation.link(source, Invalid::get_invalid()));
  auto after = operation.fold();
  EXPECT(selects(after, folded));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, dynamic_input_keeps_later_reachable) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression dynamic("dynamic"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation failing(
      domain, "failing"_view, type, child_inputs, folded, True);
  Static::Vector<Reference<Expression>, 2> inputs = {{dynamic, failing}};
  TestOperation parent(domain, "parent"_view, type, inputs, folded);

  ASSERT(parent.link(source, Invalid::get_invalid()));
  auto folded_result = parent.fold();

  EXPECT(reports(
      folded_result, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(failing.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(
    LibraryOperation,
    dynamic_predecessor_ignores_skip_decision) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  OperationExpression dynamic("dynamic"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{input}};
  TestOperation failing(
      domain, "failing"_view, type, child_inputs, folded, True);
  Static::Vector<Reference<Expression>, 2> inputs = {{dynamic, failing}};
  TestOperation parent(
      domain, "parent"_view, type, inputs, folded, False, True);

  ASSERT(parent.link(source, Invalid::get_invalid()));
  auto folded_result = parent.fold();

  EXPECT(reports(
      folded_result, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(failing.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, unreachable_input_is_never_queried) {
  Allocator::Arena domain;
  OperationMonograph source(domain);
  Types::Unsigned_64 type;
  auto& first = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Reference<Expression>, 1> child_inputs = {{first}};
  TestOperation unreachable(
      domain, "unreachable"_view, type, child_inputs, folded, True);
  Static::Vector<Reference<Expression>, 2> inputs = {{first, unreachable}};
  TestOperation parent(
      domain, "parent"_view, type, inputs, folded, False, True);

  ASSERT(parent.link(source, Invalid::get_invalid()));
  auto folded_result = parent.fold();

  EXPECT(selects(folded_result, folded));
  EXPECT(unreachable.get_evaluations() == 0);
}
