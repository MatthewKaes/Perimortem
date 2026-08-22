// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/assignment.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static constexpr Ttx::Model::Layouts::Fluid assignment_layout;

auto Language::Operations::Assignment::parse(
    const Abstract& context,
    Cursor& cursor,
    Model::Pack& left,
    Span left_span) -> Option<Expression&> {
  auto target = left.select<Expression>();
  if (!target) {
    cursor.create_expression_error(
        left_span, "Library assignment requires one Expression target."_view,
        "Assign through one writable expression rather than composed Pack "
        "flow."_view);
    return {};
  }

  Token operation = cursor.require(
      Code::Type::Assign,
      "Library Assignment requires the unambiguous `=` operator."_view);
  BAIL_IF(!operation);
  Token right_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto right = Parser::Expression::parse_write_operand(context, cursor);
  if (!right) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(operation)),
          "Library assignment requires one right operand."_view,
          "Write one complete value flow after `=`."_view);
    }
    return {};
  }

  Anchor anchor =
      Anchor::create(operation, left_span, Span(right_start, cursor.peek(-1)));
  return Expression::create_authored<Assignment>(
      cursor.get_arena(), anchor, [&](auto authored) -> Assignment {
        return Assignment(*target, *right, authored);
      });
}

auto Language::Operations::Assignment::link(
    Cursor& cursor,
    const Abstract& lexical_context,
    Option<const Abstract&> access_scope) -> Bool {
  if (linked) {
    return True;
  }

  auto selected_scope = access_scope.visit(
      []() -> Option<const Model::Type&> { return {}; },
      [](const Abstract& candidate) -> Option<const Model::Type&> {
        return candidate.select<Model::Type>();
      });
  if (!selected_scope) {
    cursor.create_expression_error(
        get_anchor(), "Assignment requires one Library Type authority."_view,
        "Retain the enclosing Function host while linking this expression."_view);
    return False;
  }

  // The receiving Expression owns target linking and complete Pack admission.
  // Assignment never needs to recover its concrete storage carrier.
  BAIL_IF(!target.link_write(cursor, lexical_context, *selected_scope, source));

  linked = True;
  return True;
}

auto Language::Operations::Assignment::finalize(Cursor& cursor) -> void {
  // Finalization follows the same independent edges fixed during linking.
  target.finalize(cursor);
  source.finalize(cursor);
}

auto Language::Operations::Assignment::lower(Llvm::Builder& body) const
    -> Bool {
  Bool target_lowered = target.lower_write_target(body);
  if (!target_lowered) {
    return False;
  }

  Bool source_lowered = source.lower(body);
  if (!source_lowered) {
    return False;
  }

  return body.write(Llvm::Builder::Write::Assign, *this, target, source);
}

auto Language::Operations::Assignment::get_value_type(Count) const
    -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Operations::Assignment::get_layout() const -> const Layout& {
  return assignment_layout;
}

auto Language::Operations::Assignment::resolve() const -> const Abstract& {
  return linked ? static_cast<const Model::Pack&>(*this)
                : static_cast<const Abstract&>(Invalid::get_invalid());
}
