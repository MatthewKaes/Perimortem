// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
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

class NotMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  NotMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "NotMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    NotMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class NotExpression : public Expression {
 public:
  NotExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

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

static auto selected(
    const Result<Option<Expression&>, Expression::Error>& result)
    -> Option<Expression&> {
  return result.visit(
      [](const Option<Expression&>& folded) -> Option<Expression&> {
        return folded.visit(
            []() -> Option<Expression&> { return {}; },
            [](Expression& selected) -> Option<Expression&> {
              return selected;
            });
      },
      [](const Expression::Error&) -> Option<Expression&> { return {}; });
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
  NotMonograph source(domain);
  Materializations materializations(domain);
  Types::Boolean distinct_bool;
  Types::Signed_8 signed_8;
  NotExpression canonical(
      "canonical"_view, Tetrodotoxin::Library::Dialect::get_bool());
  NotExpression distinct("distinct"_view, distinct_bool);
  NotExpression signed_value("signed"_view, signed_8);
  NotExpression unresolved("unresolved"_view, Invalid::get_invalid());
  auto& canonical_not =
      Operations::Not::create_synthetic(domain, materializations, canonical);
  auto& distinct_not =
      Operations::Not::create_synthetic(domain, materializations, distinct);
  auto& signed_not =
      Operations::Not::create_synthetic(domain, materializations, signed_value);
  auto& invalid_not =
      Operations::Not::create_synthetic(domain, materializations, unresolved);

  EXPECT(canonical_not.get_type().resolve().is<Invalid>());
  EXPECT_NOT(canonical_not.get_anchor());
  EXPECT(link_operation(canonical_not, source, materializations));
  EXPECT(!link_operation(distinct_not, source, materializations));
  EXPECT(!link_operation(signed_not, source, materializations));
  EXPECT(!link_operation(invalid_not, source, materializations));

  auto canonical_result = selected(canonical_not.fold());

  EXPECT_NOT(canonical_result);
  EXPECT(
      &canonical_not.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(distinct_not.get_type().resolve().is<Invalid>());
  EXPECT(signed_not.get_type().resolve().is<Invalid>());
  EXPECT(invalid_not.get_type().resolve().is<Invalid>());
  EXPECT(input_is(canonical_not, canonical));
  EXPECT(canonical_not.get_inputs().get_size() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryNot, canonical_folding) {
  Allocator::Arena domain;
  NotMonograph source(domain);
  Materializations materializations(domain);
  auto& true_value = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& false_value = Constants::False::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& complete_true = Constants::Flag::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool(), True);
  auto& complete_false = Constants::Flag::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool(), False);
  auto& true_not =
      Operations::Not::create_synthetic(domain, materializations, true_value);
  auto& false_not =
      Operations::Not::create_synthetic(domain, materializations, false_value);
  auto& complete_true_not = Operations::Not::create_synthetic(
      domain, materializations, complete_true);
  auto& complete_false_not = Operations::Not::create_synthetic(
      domain, materializations, complete_false);

  EXPECT(true_not.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(true_not, source, materializations));
  EXPECT(link_operation(false_not, source, materializations));
  EXPECT(link_operation(complete_true_not, source, materializations));
  EXPECT(link_operation(complete_false_not, source, materializations));

  auto true_result = selected(true_not.fold());
  auto false_result = selected(false_not.fold());
  auto complete_true_result = selected(complete_true_not.fold());
  auto complete_false_result = selected(complete_false_not.fold());

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
  NotMonograph source(domain);
  Materializations materializations(domain);
  auto& true_value = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& child =
      Operations::Not::create_synthetic(domain, materializations, true_value);
  auto& parent =
      Operations::Not::create_synthetic(domain, materializations, child);

  EXPECT(parent.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(parent, source, materializations));
  EXPECT(link_operation(parent, source, materializations));

  auto parent_result = selected(parent.fold());
  auto repeated_result = selected(parent.fold());

  ASSERT(parent_result && repeated_result);
  EXPECT(parent_result->is<Constants::True>());
  EXPECT(&*parent_result == &*repeated_result);
  EXPECT(
      &parent_result->get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
}
