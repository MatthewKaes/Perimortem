// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/constant.hpp"

#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem::Core;

auto Ttx::Concept::Constant::prove(Abstract& candidate) -> Option<Constant&> {
  ttx_constant_view view;
  return ttx_constant_prove(candidate.get_abi(), &view)
             ? Option<Constant&>(static_cast<Constant&>(candidate))
             : Option<Constant&>();
}

auto Ttx::Concept::Constant::prove(const Abstract& candidate)
    -> Option<const Constant&> {
  ttx_constant_view view;
  return ttx_constant_prove(candidate.get_abi(), &view)
             ? Option<const Constant&>(static_cast<const Constant&>(candidate))
             : Option<const Constant&>();
}

auto Ttx::Concept::Constant::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  return name == "fold"_view
             ? static_cast<const Abstract&>(*this)
             : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Ttx::Concept::Constant::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Abstract::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, *this);
}

auto Ttx::Concept::Constant::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_constant_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), ttx_interface_marker_operations())
             : Abstract::negotiate_interface(requirement);
}
