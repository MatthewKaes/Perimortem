// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/option.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

static auto payloads_equal(const Model::Pack& left, const Model::Pack& right)
    -> Bool {
  const Layout& left_layout = left.get_layout();
  const Layout& right_layout = right.get_layout();
  if (left_layout.get_size() != right_layout.get_size()) {
    return False;
  }

  for (Count index = 0; index < left_layout.get_size(); index++) {
    auto left_entry = left_layout.get_abstract(index);
    auto right_entry = right_layout.get_abstract(index);
    auto left_constant = left_entry.visit(
        []() -> Core::Option<const Constant&> { return {}; },
        [](const Abstract& selected) { return selected.select<Constant>(); });
    auto right_constant = right_entry.visit(
        []() -> Core::Option<const Constant&> { return {}; },
        [](const Abstract& selected) { return selected.select<Constant>(); });
    if (!left_constant || !right_constant ||
        *left_constant != *right_constant) {
      return False;
    }
  }

  return True;
}

auto Constants::Option::create_absent(
    Memory::Allocator::Arena& domain,
    const Types::Option& type) -> Option& {
  return Expression::create_synthetic<Option>(
      domain, [&](auto source) -> Option {
        return Option(type, Types::Option::Kind::Absent, {}, source);
      });
}

auto Constants::Option::create_present(
    Memory::Allocator::Arena& domain,
    const Types::Option& type,
    Model::Pack& payload) -> Core::Option<Option&> {
  BAIL_IF(!payload.fits_into(type.get_element_type()));
  const Layout& layout = payload.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    BAIL_IF(!entry || !entry->is<Constant>());
  }

  return Expression::create_synthetic<Option>(
      domain, [&](auto source) -> Option {
        return Option(
            type, Types::Option::Kind::Present,
            Ttx::Concept::Reference<Model::Pack>(payload), source);
      });
}

auto Constants::Option::create_fitted(
    Memory::Allocator::Arena& domain,
    const Types::Option& type,
    Model::Pack& source) -> Core::Option<Option&> {
  auto retained = source.select<Constants::Option>();
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }
  if (source.get_layout().is_empty()) {
    return create_absent(domain, type);
  }

  Model::Pack* payload = &source;
  auto expression = source.select<Expression>();
  if (expression) {
    Core::Option<Model::Pack&> folded;
    expression->fold().visit(
        [&](const Core::Option<Model::Pack&>& selected) { folded = selected; },
        [](const Expression::Error&) {});
    BAIL_IF(!folded);
    payload = &*folded;
  }

  retained = payload->select<Constants::Option>();
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  return create_present(domain, type, *payload);
}

auto Constants::Option::get_payload() const
    -> Core::Option<const Model::Pack&> {
  return payload.visit(
      []() -> Core::Option<const Model::Pack&> { return {}; },
      [](const Reference<Model::Pack>& selected)
          -> Core::Option<const Model::Pack&> { return selected.get(); });
}

auto Constants::Option::equals(const Constant& rhs) const -> Bool {
  auto selected = rhs.select<Constants::Option>();
  if (!selected || !has_same_type(rhs) || kind != selected->kind) {
    return False;
  }
  if (kind == Types::Option::Kind::Absent) {
    return True;
  }

  auto left_payload = get_payload();
  auto right_payload = selected->get_payload();
  return left_payload && right_payload &&
                 payloads_equal(*left_payload, *right_payload)
             ? True
             : False;
}
