// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/assignment.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/index.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/foreign/state.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto is_assignment_operator(Code::Type code) -> Bool {
  return code == Code::Type::Assign || code == Code::Type::AddAssign ||
         code == Code::Type::SubAssign;
}

static auto is_assignment_syntax(const Language::Expression& expression)
    -> Bool {
  return expression.visit<Language::Expressions::Identifier>(
      [](const Language::Expressions::Identifier& identifier) {
        Code code = identifier.get_token().get_code();
        return Bool(
            code == Code::Type::Addressable || code == Code::Type::Self ||
            code == Code::Type::Type);
      },
      [](const Abstract& not_identifier) {
        return not_identifier.visit<Language::Access::Address>(
            [](const Language::Access::Address& address) {
              return is_assignment_syntax(address.get_receiver());
            },
            [](const Abstract& not_address) {
              return not_address.visit<Language::Access::Index>(
                  [](const Language::Access::Index& index) {
                    return is_assignment_syntax(index.get_receiver());
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

static auto is_writable(
    const Addressable& addressable,
    const Type& access_scope) -> Bool {
  return addressable.visit<Language::Flow::Local>(
      [](const Language::Flow::Local& local) {
        return Bool(local.get_writability() == Language::Writability::Full);
      },
      [&](const Abstract& not_local) {
        return not_local.visit<Language::Foreign::State>(
            [](const Language::Foreign::State& state) {
              // Exposed State is readable through dot but only public State
              // grants the parent Library a write path.
              return Bool(
                  state.get_visibility() ==
                  Tetrodotoxin::Language::Visibility::Public);
            },
            [&](const Abstract& not_foreign) {
              return not_foreign.visit<Language::Field>(
                  [&](const Language::Field& field) {
                    switch (field.get_writability()) {
                    case Language::Writability::Full:
                      return True;
                    case Language::Writability::Internal:
                      if (field.get_definition().get_visibility() ==
                          Tetrodotoxin::Language::Visibility::Public) {
                        return True;
                      }
                      return field.get_host().visit<Language::Types::Composite>(
                          [&](const Language::Types::Composite& composite) {
                            return composite.grants_private_access(
                                access_scope);
                          },
                          [](const Abstract&) { return False; });
                    case Language::Writability::Constant:
                      return False;
                    }
                    return False;
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

static auto is_numeric(const Type& type) -> Bool {
  return type.is<Ttx::Model::Types::Signed>() ||
         type.is<Ttx::Model::Types::Unsigned>() ||
         type.is<Ttx::Model::Types::Real>();
}

auto Language::Flow::Assignment::interpret(
    Memory::Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor) -> Core::Option<Assignment&> {
  // A Type token may begin only a Static Field path. The completed target must
  // still resolve to Addressable, so a bare Type never becomes writable.
  Code start = cursor.get_code();
  if (start != Code::Type::Addressable && start != Code::Type::Self &&
      start != Code::Type::Type) {
    return {};
  }

  auto transaction = cursor.branch();
  Token opening = transaction.current();
  auto parsed = Parser::Expression::parse(domain, source, transaction);
  auto target = parsed.visit(
      []() -> Core::Option<Expression&> { return {}; },
      [](Model::Pack& selected) { return selected.select<Expression>(); });
  BAIL_IF(
      !target || !is_assignment_operator(transaction.get_code().get_type()));

  Token operation = transaction.consume();
  if (!is_assignment_syntax(*target)) {
    transaction.create_expression_error(
        Anchor::create(operation, Span(opening, operation)),
        "Library assignment requires an addressable target path."_view,
        "Start with one Addressable, Type, or `self` and use only `.` or `[]` "
        "selection."_view);
    return {};
  }

  auto value = Model::Parser::Pack::parse(domain, source, transaction);
  BAIL_IF(!value);
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library assignment requires one terminating `;`."_view);
  BAIL_IF(!terminator);

  Assignment& assignment = domain.construct_from<Assignment>([&]() {
    return Assignment(
        *target, *value, operation.get_code().get_type(),
        Anchor::create(operation, Span(opening, terminator)));
  });
  cursor.join(transaction);
  return assignment;
}

auto Language::Flow::Assignment::link(
    Tetrodotoxin::Language::Monograph& monograph,
    const Abstract& lexical_context,
    const Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }

  BAIL_IF(!target.link(monograph, lexical_context, access_scope));
  BAIL_IF(!source.link(monograph, lexical_context, access_scope));

  const Type* target_type = nullptr;
  auto index = target.select<Access::Index>();
  if (index) {
    auto element_type = index->get_element_type().select<Type>();
    if (element_type) {
      target_type = &*element_type;
    }
  } else {
    auto addressable = target.get_result().select<Addressable>();
    if (addressable && is_writable(*addressable, access_scope)) {
      target_type = &addressable->get_type();
    }
  }

  if (target_type == nullptr) {
    monograph.report(
        anchor,
        "Assignment target is not one writable Library Addressable."_view,
        "Use mutable Local or Field storage, public Foreign State, or an "
        "indexed Access address."_view);
    return False;
  }

  if (operation == Code::Type::Assign) {
    if (!source.fits_into(*target_type)) {
      monograph.report(
          anchor,
          "Assignment source Pack does not fit the target Type Layout."_view,
          "Supply the complete value flow required by the selected target."_view);
      return False;
    }
  } else {
    auto expression = source.select<Expression>();
    const Abstract& source_type =
        expression ? expression->get_type().resolve() : Invalid::get_invalid();
    if (!expression || &source_type != target_type ||
        !is_numeric(*target_type)) {
      monograph.report(
          anchor,
          "Compound assignment requires one exact matching numeric value."_view,
          "Use the target Type on both sides of `+=` or `-=`."_view);
      return False;
    }
  }

  linked = True;
  return True;
}

auto Language::Flow::Assignment::finalize() -> void {
  target.finalize();
  source.finalize();
}
