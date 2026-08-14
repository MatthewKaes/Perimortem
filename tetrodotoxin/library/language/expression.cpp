// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expression.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_output_type(const Abstract& candidate)
    -> Option<const Type&> {
  auto direct = candidate.select<Type>();
  return direct ? direct : candidate.resolve().select<Type>();
}

auto Language::Expression::get_layout() const -> const Layout& {
  auto type = select_output_type(get_type());
  if (!type) {
    // Layout observation is legal only after resolve() proves this Pack. An
    // empty Layout is completed zero value flow, so returning it here would
    // silently turn an incomplete Expression into a valid empty producer.
    __builtin_trap();
  }

  if (type->get_layout().is_empty()) {
    __builtin_trap();
  }

  return output_layout;
}

auto Language::Expression::resolve() const -> const Abstract& {
  auto type = select_output_type(get_type());
  if (!type || type->get_layout().is_empty()) {
    return Invalid::get_invalid();
  }

  return static_cast<const Language::Model::Pack&>(*this);
}

auto Language::Expression::finalize() -> void {
  fold();
}

auto Language::Expression::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract&,
    Option<const Type&>) -> Bool {
  auto source_anchor = get_anchor();

  // A declaration pass may already expose one exact Composite Type while that
  // Type still resolves Invalid until its own Layout is complete. Expression
  // linking retains that real output edge. It does not make unrelated Type
  // completion a prerequisite for selecting the value's domain.
  auto type = select_output_type(get_type());
  if (type && !type->get_layout().is_empty()) {
    return True;
  }

  source.report(
      source_anchor, "Expression did not resolve one nonempty Type."_view,
      "Complete its semantic inputs or keep empty output as Pack flow."_view);
  return False;
}

auto Language::Expression::Error::get_name() const -> View::Bytes {
  switch (type) {
  case Type::InvalidOperationType:
    return "invalid operation Type"_view;
  case Type::InvalidInput:
    return "invalid operation input"_view;
  case Type::InvalidConstant:
    return "invalid Constant domain"_view;
  case Type::ResultTypeMismatch:
    return "changed result Type"_view;
  case Type::ArithmeticOverflow:
    return "arithmetic overflow"_view;
  case Type::DivisionByZero:
    return "division by zero"_view;
  case Type::Unknown:
    return "unknown fold failure"_view;
  }

  return "unknown fold failure"_view;
}

auto Language::Expression::fold() -> Perimortem::Utility::
    Result<Perimortem::Core::Option<Language::Model::Pack&>, Error> {
  if (!folded.is_null()) {
    return folded.visit(
        []() -> Perimortem::Utility::Result<
                 Perimortem::Core::Option<Language::Model::Pack&>, Error> {
          return Perimortem::Core::Option<Language::Model::Pack&>{};
        },
        [](Language::Model::Pack& representation)
            -> Perimortem::Utility::Result<
                Perimortem::Core::Option<Language::Model::Pack&>, Error> {
          return representation;
        },
        [](const Error& error)
            -> Perimortem::Utility::Result<
                Perimortem::Core::Option<Language::Model::Pack&>, Error> {
          return error;
        });
  }

  if (&resolve() == &Invalid::get_invalid()) {
    return Perimortem::Core::Option<Language::Model::Pack&>{};
  }

  auto result = evaluate();
  return result.visit(
      [&](const Perimortem::Core::Option<Language::Model::Pack&>& selected)
          -> Perimortem::Utility::Result<
              Perimortem::Core::Option<Language::Model::Pack&>, Error> {
        if (!selected) {
          return Perimortem::Core::Option<Language::Model::Pack&>{};
        }

        Language::Model::Pack& representation = *selected;
        const Layout& representation_layout = representation.get_layout();
        Bool constants = True;
        for (Count index = 0; index < representation_layout.get_size();
             index++) {
          auto entry = representation_layout.get_abstract(index);
          constants &= Bool(entry && entry->is<Constant>());
        }
        if (!constants) {
          Error error(Error::Type::InvalidConstant, *this);
          folded = error;
          return error;
        }

        const Layout& source_layout = get_layout();
        Bool exact_shape =
            source_layout.get_size() == representation_layout.get_size();
        if (exact_shape && source_layout.get_size() == 1) {
          const Abstract& expression_type = get_type().resolve();
          const Abstract& result_type = representation.get_type().resolve();
          exact_shape = expression_type.is<Type>() && result_type.is<Type>() &&
                        &expression_type == &result_type;
        } else if (exact_shape) {
          // The authored Layout owns the output promise. A folded Pack may
          // replace a repeated producer (such as one Slice identity) with its
          // concrete Constant entries, so the reverse fit is not meaningful:
          // it would ask those Constants to reproduce the source owner.
          exact_shape = source_layout.fits(representation_layout);
        }
        if (!exact_shape) {
          Error error(Error::Type::ResultTypeMismatch, *this);
          folded = error;
          return error;
        }

        folded = representation;
        return Perimortem::Core::Option<Language::Model::Pack&>(representation);
      },
      [&](const Error& error)
          -> Perimortem::Utility::Result<
              Perimortem::Core::Option<Language::Model::Pack&>, Error> {
        folded = error;
        return error;
      });
}

auto Language::Expression::get_folded()
    -> Perimortem::Core::Option<Language::Model::Pack&> {
  return folded.visit(
      []() -> Perimortem::Core::Option<Language::Model::Pack&> { return {}; },
      [](Language::Model::Pack& representation)
          -> Perimortem::Core::Option<Language::Model::Pack&> {
        return representation;
      },
      [](const Error&) -> Perimortem::Core::Option<Language::Model::Pack&> {
        return {};
      });
}

auto Language::Expression::get_folded() const
    -> Perimortem::Core::Option<const Language::Model::Pack&> {
  return folded.visit(
      []() -> Perimortem::Core::Option<const Language::Model::Pack&> {
        return {};
      },
      [](const Language::Model::Pack& representation)
          -> Perimortem::Core::Option<const Language::Model::Pack&> {
        return representation;
      },
      [](const Error&)
          -> Perimortem::Core::Option<const Language::Model::Pack&> {
        return {};
      });
}

auto Language::Expression::evaluate() -> Perimortem::Utility::
    Result<Perimortem::Core::Option<Language::Model::Pack&>, Error> {
  if (is<Constant>()) {
    return static_cast<Language::Model::Pack&>(*this);
  }

  const Abstract& result = get_result();
  return result.visit<Language::Field>(
      [](const Language::Field& field) { return field.get_constant(); },
      [](const Abstract& candidate) {
        return candidate.visit<Language::Flow::Local>(
            [](const Language::Flow::Local& local) {
              return local.get_constant();
            },
            [](const Abstract&) -> Option<Language::Model::Pack&> {
              return {};
            });
      });
}
