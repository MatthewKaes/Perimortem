// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/propagate.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/fold.hpp"
#include "tetrodotoxin/library/language/model/propagation.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Access::Propagate::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Anchor anchor) -> Propagate& {
  Model::Pack& empty_escape = Model::Pack::create_empty(domain);
  return Expression::create_authored<Propagate>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Propagate {
        return Propagate(receiver, empty_escape, source);
      });
}

auto Language::Access::Propagate::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  auto selected_type = receiver.get_type().resolve().select<Model::Type>();
  auto propagation = selected_type
                         ? selected_type->resolve_concept("propagation"_view)
                               .select<Model::Propagation>()
                         : Core::Option<const Model::Propagation&>();
  if (!selected_type || !propagation) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` requires a propagating value Type."_view,
        "Use Option, Bool, Result, or another Type that defines propagation."_view);
    return False;
  }

  auto scope = lexical_context.select<Flow::Scope>();
  if (receiver_type && *receiver_type != &*selected_type) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` selected a different receiver Type."_view,
        "Repeat linking with the same completed receiver identity."_view);
    return False;
  }

  const Model::Type& propagated = propagation->continuation();
  auto propagated_error = propagation->escape();
  if (propagated_error) {
    if (error_type && *error_type != &*propagated_error) {
      cursor.create_expression_error(
          get_anchor(),
          "Postfix `?` selected a different propagated error Type."_view,
          "Repeat linking with the same completed receiver Type."_view);
      return False;
    }

    if (!error_type) {
      ErrorEscape& created = Expression::create_synthetic<ErrorEscape>(
          cursor.get_arena(),
          [&](Core::Option<Anchor>) { return ErrorEscape(*propagated_error); });
      escape = &created;
      error_type = &*propagated_error;
    }
  } else if (error_type) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` changed its propagated escape shape."_view,
        "Repeat linking with the same completed receiver Type."_view);
    return False;
  }

  Model::Pack& selected_escape = *escape;
  BAIL_IF(!selected_escape.link(cursor, lexical_context, access_scope));
  if (!scope || !selected_escape.fits(scope->get_function_results())) {
    auto report = cursor.create_report(get_anchor());
    report
        << "Postfix `?` escape values do not fit the Function result Layout.\n"
           "Escape produces: "_view;
    Language::Diagnostics::write_pack(report, selected_escape);
    report << "\nFunction accepts: "_view;
    if (scope) {
      Language::Diagnostics::write_layout(
          report, scope->get_function_results());
    } else {
      report << "<no Function scope>"_view;
    }
    if (error_type) {
      report.get_hint()
          << "Return the propagated error Type or a receiving Result with that "
             "exact error Type."_view;
    } else {
      report.get_hint()
          << "Use an empty Function result or one receiving Option result."_view;
    }
    return False;
  }

  if (continuation_type && *continuation_type != &propagated) {
    cursor.create_expression_error(
        get_anchor(),
        "Postfix `?` selected a different continuation Type."_view,
        "Repeat linking with the same completed receiver identity."_view);
    return False;
  }

  receiver_type = &*selected_type;
  continuation_type = &propagated;
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Propagate::get_type() const -> const Abstract& {
  return continuation_type.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Language::Model::Type* selected) -> const Abstract& {
        return *selected;
      });
}

auto Language::Access::Propagate::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name != "fold"_view) {
    return Expression::resolve_concept(name);
  }
  return const_cast<Propagate&>(*this).evaluate_fold().visit(
      [](const Core::Option<Model::Pack&>& result) -> const Abstract& {
        return fold_answer(result);
      },
      [](const Expression::Error&) -> const Abstract& {
        return Ttx::Concept::None::get_none();
      });
}

auto Language::Access::Propagate::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Expression::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, resolve_concept("fold"_view));
}

auto Language::Access::Propagate::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  escape->finalize(cursor);
  Expression::finalize(cursor);
}

auto Language::Access::Propagate::evaluate_fold()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  const Abstract& folded = query_fold(receiver);
  if (!Ttx::Concept::Constant::prove(folded)) {
    return Core::Option<Model::Pack&>{};
  }
  const Abstract& propagated = folded.resolve_concept("propagate"_view);
  if (&propagated == &Ttx::Concept::None::get_none()) {
    return Core::Option<Model::Pack&>{};
  }
  if (!Ttx::Concept::Constant::prove(propagated)) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }
  auto pack = Model::Pack::from(const_cast<Abstract&>(propagated));
  return pack ? Core::Option<Model::Pack&>(*pack)
              : Core::Option<Model::Pack&>();
}
