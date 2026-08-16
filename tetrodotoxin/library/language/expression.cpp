// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expression.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto select_output_type(const Abstract& candidate)
    -> Option<const Language::Model::Type&> {
  auto direct = candidate.select<Language::Model::Type>();
  return direct ? direct : candidate.resolve().select<Language::Model::Type>();
}

static auto select_value_type(const Abstract& candidate)
    -> Option<const Language::Model::Type&> {
  auto type = select_output_type(candidate);
  BAIL_IF(!type || type->get_layout().is_empty());
  return *type;
}

static constexpr Ttx::Model::Layouts::Fluid empty_expression_layout;

auto Language::Expression::get_layout() const -> const Layout& {
  auto type = select_value_type(get_type());
  // Layout inspection is total even when this Expression has no value output.
  // Pack resolution remains the separate admission proof, so this empty shape
  // cannot fit storage or masquerade as completed zero value flow.
  if (!type || type->get_layout().is_empty()) {
    return empty_expression_layout;
  }
  return output_layout;
}

auto Language::Expression::resolve() const -> const Abstract& {
  auto type = select_value_type(get_type());
  if (!type) {
    return Invalid::get_invalid();
  }

  return static_cast<const Language::Model::Pack&>(*this);
}

auto Language::Expression::get_value_type(Count index) const
    -> const Abstract& {
  if (index != 0) {
    return Invalid::get_invalid();
  }
  return select_value_type(get_type())
      .visit(
          []() -> const Abstract& { return Invalid::get_invalid(); },
          [](const Language::Model::Type& type) -> const Abstract& {
            return type;
          });
}

auto Language::Expression::finalize(Ttx::Lexical::Cursor&) -> void {
  fold();
}

auto Language::Expression::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract&,
    Option<const Abstract&>) -> Bool {
  auto source_anchor = get_anchor();

  // A declaration Type's identity and immutable Layout are available before
  // its recursive closure settles. Admitting that exact edge here preserves
  // forward references while the Monograph barrier still prevents an invalid
  // Type or Expression from escaping the source transaction.
  auto type = select_value_type(get_type());
  if (type) {
    return True;
  }

  cursor.create_expression_error(
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
          exact_shape = expression_type.is<Language::Model::Type>() &&
                        result_type.is<Language::Model::Type>() &&
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

auto Language::Expression::get_write_type(
    const Language::Model::Type& access_scope) const
    -> Option<const Language::Model::Type&> {
  auto addressable =
      get_result().resolve().select<Language::Model::Addressable>();
  return addressable && addressable->permits_write_from(access_scope)
             ? Option<const Language::Model::Type&>(addressable->get_type())
             : Option<const Language::Model::Type&>();
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
  auto addressable = result.resolve().select<Language::Model::Addressable>();
  return addressable ? addressable->get_constant()
                     : Option<Language::Model::Pack&>();
}
