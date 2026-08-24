// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/interfaces/callable.hpp"

#include "ttx/model/callable.hpp"

using namespace Ttx;

auto Model::Interfaces::Callable::negotiate(
    const Concept::Abstract& requirement,
    const Concept::Abstract& candidate) const -> Relation {
  auto required = requirement.resolve().select<Model::Callable>();
  auto supplied = candidate.resolve().select<Model::Callable>();
  if (!required || !supplied) {
    return Relation::Rejected;
  }

  Bool parameters_agree =
      required->get_parameters().fits(supplied->get_parameters()) &&
      supplied->get_parameters().fits(required->get_parameters());
  Bool results_satisfy = supplied->get_results().fits(required->get_results());
  if (!parameters_agree || !results_satisfy) {
    return Relation::Rejected;
  }

  return required->get_results().fits(supplied->get_results())
             ? Relation::Equivalent
             : Relation::Satisfied;
}
