// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/add_assignment.hpp"

#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static constexpr Ttx::Model::Layouts::Fluid assignment_layout;

auto Language::Operations::AddAssignment::parse(
    const Abstract& context,
    Cursor& cursor,
    Model::Pack& left,
    Span left_span) -> Option<Expression&> {
  auto target = left.select<Expression>();
  if (!target) {
    cursor.create_expression_error(
        left_span,
        "Library addition assignment requires one Expression target."_view,
        "Apply `+=` through one writable scalar expression."_view);
    return {};
  }

  Token operation = cursor.require(
      Code::Type::AddAssign,
      "Library AddAssignment requires the unambiguous `+=` operator."_view);
  BAIL_IF(!operation);
  Token right_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto parsed_right = Parser::Expression::parse_write_operand(context, cursor);
  if (!parsed_right) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(operation)),
          "Library addition assignment requires one right operand."_view,
          "Write one scalar value after `+=`."_view);
    }
    return {};
  }

  auto right = parsed_right->select<Expression>();
  Anchor anchor =
      Anchor::create(operation, left_span, Span(right_start, cursor.peek(-1)));
  if (!right) {
    cursor.create_expression_error(
        anchor,
        "Library addition assignment requires one Expression value."_view,
        "Use one unlabelled scalar operand for `+=`."_view);
    return {};
  }

  return Expression::create_authored<AddAssignment>(
      cursor.get_arena(), anchor, [&](auto authored) -> AddAssignment {
        return AddAssignment(*target, *right, authored);
      });
}

auto Language::Operations::AddAssignment::link(
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
        get_anchor(),
        "Addition assignment requires one Library Type authority."_view,
        "Retain the enclosing Function host while linking this expression."_view);
    return False;
  }

  // Scalar compound assignment asks the same target operation to bind its
  // write edge while retaining the target Type for the required read.
  BAIL_IF(!target.link_write(cursor, lexical_context, *selected_scope, right));

  auto target_type = target.get_write_type(*selected_scope);
  const Abstract& read_type = target.get_type().resolve();
  const Abstract& right_type = right.get_type().resolve();
  Bool numeric = target_type && (target_type->is<Model::Types::Unsigned>() ||
                                 target_type->is<Model::Types::Signed>() ||
                                 target_type->is<Model::Types::Real>());
  if (!target_type || &read_type != &*target_type ||
      &right_type != &*target_type || !numeric) {
    cursor.create_expression_error(
        get_anchor(),
        "Addition assignment requires one writable exact numeric Type."_view,
        "Use the same signed, unsigned, or real Type on both sides of `+=`."_view);
    return False;
  }

  linked = True;
  return True;
}

auto Language::Operations::AddAssignment::finalize(Cursor& cursor) -> void {
  target.finalize(cursor);
  right.finalize(cursor);
}

auto Language::Operations::AddAssignment::lower(Llvm::Builder& body) const
    -> Bool {
  Bool target_lowered = target.lower_write_target(body);
  if (!target_lowered) {
    return False;
  }

  Bool right_lowered = right.lower(body);
  if (!right_lowered) {
    return False;
  }

  return body.write(Llvm::Builder::Write::Add, *this, target, right);
}

auto Language::Operations::AddAssignment::get_value_type(Count) const
    -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Operations::AddAssignment::get_layout() const -> const Layout& {
  return assignment_layout;
}

auto Language::Operations::AddAssignment::resolve() const -> const Abstract& {
  return linked ? static_cast<const Model::Pack&>(*this)
                : static_cast<const Abstract&>(Invalid::get_invalid());
}
