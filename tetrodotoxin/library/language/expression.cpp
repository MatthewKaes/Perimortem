// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expression.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "ttx/bootstrap/model/layouts/fluid.hpp"
#include "ttx/bootstrap/model/layouts/named.hpp"

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

auto Language::Expression::Error::from_pack(
    Type type,
    const Language::Model::Pack& pack) -> Error {
  auto identity = pack.get_identity();
  return Error(type, identity ? *identity : Unknown::get_unknown());
}

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
    return Unknown::get_unknown();
  }

  return *this;
}

auto Language::Expression::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  if (name == "expression"_view) {
    return *this;
  }
  return Abstract::resolve_concept(name);
}

auto Language::Expression::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  visit_concept(visitor, "expression"_view, *this);
}

auto Language::Expression::get_value_type(Count index) const
    -> const Abstract& {
  if (index != 0) {
    return Unknown::get_unknown();
  }
  return select_value_type(get_type())
      .visit(
          []() -> const Abstract& { return Unknown::get_unknown(); },
          [](const Language::Model::Type& type) -> const Abstract& {
            return type;
          });
}

auto Language::Expression::finalize(Ttx::Lexical::Cursor&) -> void {}

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
    return "invalid Tetrodotoxin::Library::Language::Constant domain"_view;
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

auto Language::Expression::get_write_type(
    const Language::Model::Type& access_scope) const
    -> Option<const Language::Model::Type&> {
  auto addressable =
      get_result().resolve().select<Language::Model::Addressable>();
  auto type = addressable
                  ? addressable->get_type().select<Language::Model::Type>()
                  : Option<const Language::Model::Type&>();
  return addressable && type && addressable->permits_write_from(access_scope)
             ? type
             : Option<const Language::Model::Type&>();
}

auto Language::Expression::link_write(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope,
    Language::Model::Pack& source) -> Bool {
  Bool failed = !link_write_target(cursor, lexical_context, access_scope);
  failed |= !source.link(cursor, lexical_context, access_scope);
  BAIL_IF(failed);

  if (!source.is_complete()) {
    auto report = cursor.create_report(get_anchor());
    auto source_identity = source.get_identity();
    report << "Cannot write incomplete source '"_view
           << (source_identity ? source_identity->get_name() : "Pack"_view)
           << "' to target '"_view << get_name() << "'."_view;
    report.get_hint()
        << "Fix the source expression before assigning its value."_view;
    return False;
  }

  if (!accepts_write(source, access_scope)) {
    auto report = cursor.create_report(get_anchor());
    report << "Cannot write source values to target '"_view << get_name()
           << "'.\nSource produces: "_view;
    Language::Diagnostics::write_pack(report, source);
    auto target_type = get_write_type(access_scope);
    if (target_type) {
      report << "\nTarget '"_view << target_type->get_name()
             << "' accepts: "_view;
      Language::Diagnostics::write_layout(report, target_type->get_layout());
    }
    report.get_hint()
        << "Supply values with the exact count and Types shown for the target."_view;
    return False;
  }

  return True;
}

auto Language::Expression::link_write_restored(
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope,
    Language::Model::Pack& source) -> Bool {
  Bool failed = !link_write_target_restored(lexical_context, access_scope);
  failed |= !source.link_restored(lexical_context, access_scope);
  BAIL_IF(failed || !source.is_complete());
  return accepts_write(source, access_scope);
}

auto Language::Expression::link_write_target(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  return link(cursor, lexical_context, access_scope);
}

auto Language::Expression::link_write_target_restored(
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  return link_restored(lexical_context, access_scope);
}

auto Language::Expression::accepts_write(
    const Language::Model::Pack& source,
    const Language::Model::Type& access_scope) const -> Bool {
  auto target_type = get_write_type(access_scope);
  return target_type && source.fits_into(*target_type);
}
