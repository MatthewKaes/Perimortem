// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryNot = {
  .name = "Tetrodotoxin::Library::Language::Operations::Not"_view,
};

class NotExpression : public Expression {
 public:
  NotExpression(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Abstract& type;
  Ttx::Model::Layouts::Fluid inputs;
};

static auto selected(const Result<const Expression&, FoldError>& result)
    -> Option<const Expression&> {
  return result.visit(
      [](const Expression& expression) -> Option<const Expression&> {
        return expression;
      },
      [](const FoldError&) -> Option<const Expression&> { return {}; });
}

static auto input_is(
    const Operations::Not& operation,
    const Expression& expected) -> Bool {
  return operation.get_inputs().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryNot, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Boolean distinct_bool;
  Types::Signed_8 signed_8;
  NotExpression canonical(
      "canonical"_view, Tetrodotoxin::Library::Dialect::get_bool());
  NotExpression distinct("distinct"_view, distinct_bool);
  NotExpression signed_value("signed"_view, signed_8);
  NotExpression unresolved("unresolved"_view, Invalid::get_invalid());
  Operations::Not canonical_not(domain, canonical);
  Operations::Not distinct_not(domain, distinct);
  Operations::Not signed_not(domain, signed_value);
  Operations::Not invalid_not(domain, unresolved);
  auto canonical_result =
      selected(canonical_not.attempt_fold(domain, materializations));

  ASSERT(canonical_result);
  EXPECT(
      &canonical_not.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&*canonical_result == &canonical_not);
  EXPECT(distinct_not.get_type().resolve().is<Invalid>());
  EXPECT(signed_not.get_type().resolve().is<Invalid>());
  EXPECT(invalid_not.get_type().resolve().is<Invalid>());
  EXPECT(input_is(canonical_not, canonical));
  EXPECT(canonical_not.get_inputs().get_size() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryNot, canonical_folding) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Constants::True true_value(Tetrodotoxin::Library::Dialect::get_bool());
  Constants::False false_value(Tetrodotoxin::Library::Dialect::get_bool());
  Constants::Flag complete_true(
      Tetrodotoxin::Library::Dialect::get_bool(), True);
  Constants::Flag complete_false(
      Tetrodotoxin::Library::Dialect::get_bool(), False);
  Operations::Not true_not(domain, true_value);
  Operations::Not false_not(domain, false_value);
  Operations::Not complete_true_not(domain, complete_true);
  Operations::Not complete_false_not(domain, complete_false);
  auto true_result = selected(true_not.attempt_fold(domain, materializations));
  auto false_result =
      selected(false_not.attempt_fold(domain, materializations));
  auto complete_true_result =
      selected(complete_true_not.attempt_fold(domain, materializations));
  auto complete_false_result =
      selected(complete_false_not.attempt_fold(domain, materializations));

  ASSERT(
      true_result && false_result && complete_true_result &&
      complete_false_result);
  EXPECT(true_result->is<Constants::False>());
  EXPECT(false_result->is<Constants::True>());
  EXPECT(complete_true_result->is<Constants::False>());
  EXPECT(complete_false_result->is<Constants::True>());
  EXPECT(
      &true_result->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &false_result->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
}

PERIMORTEM_UNIT_TEST(LibraryNot, recursive_and_repeated_folding) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Constants::True true_value(Tetrodotoxin::Library::Dialect::get_bool());
  Operations::Not child(domain, true_value);
  Operations::Not parent(domain, child);
  auto parent_result = selected(parent.attempt_fold(domain, materializations));
  auto repeated_result =
      selected(parent.attempt_fold(domain, materializations));

  ASSERT(parent_result && repeated_result);
  EXPECT(parent_result->is<Constants::True>());
  EXPECT(&*parent_result == &*repeated_result);
  EXPECT(
      &parent_result->get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
}
