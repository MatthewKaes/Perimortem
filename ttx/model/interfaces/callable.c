// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/interfaces/callable.h"

ttx_interface_relation ttx_callable_negotiate(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate) {
  ttx_callable_view required;
  ttx_callable_view supplied;
  const ttx_layout* required_parameters;
  const ttx_layout* supplied_parameters;
  const ttx_layout* required_results;
  const ttx_layout* supplied_results;

  requirement = ttx_abstract_resolve(requirement);
  candidate = ttx_abstract_resolve(candidate);
  if (!ttx_callable_prove(requirement, &required) ||
      !ttx_callable_prove(candidate, &supplied)) {
    return TTX_INTERFACE_REJECTED;
  }
  required_parameters = ttx_callable_parameters(&required);
  supplied_parameters = ttx_callable_parameters(&supplied);
  required_results = ttx_callable_results(&required);
  supplied_results = ttx_callable_results(&supplied);
  if (!ttx_layout_fits(required_parameters, supplied_parameters) ||
      !ttx_layout_fits(supplied_parameters, required_parameters) ||
      !ttx_layout_fits(supplied_results, required_results)) {
    return TTX_INTERFACE_REJECTED;
  }
  return ttx_layout_fits(required_results, supplied_results)
             ? TTX_INTERFACE_EQUIVALENT
             : TTX_INTERFACE_SATISFIED;
}
