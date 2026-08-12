// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/signature.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Signature::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor) -> Option<Signature&> {
  auto transaction = cursor.branch();

  auto parameters = Language::Model::Layout::interpret_parameters(
      domain, source, transaction);
  BAIL_IF(!parameters);

  BAIL_IF(!transaction.require(
      Code::Type::CallOp,
      "Library Function parameters require `->` before the result "
      "Layout."_view));

  auto results =
      Language::Model::Layout::interpret(domain, source, transaction);
  BAIL_IF(!results);

  Signature& signature = domain.construct_from<Signature>(
      [&]() -> Signature { return Signature(*parameters, *results); });
  cursor.join(transaction);
  return signature;
}

auto Language::Signature::link(
    Tetrodotoxin::Language::Monograph& source,
    const Type& host) -> Bool {
  // Both models run so one malformed parameter cannot hide an independent
  // result diagnostic. Each model owns idempotence for its exact staged edges.
  Bool parameters_linked = parameters.link_parameters(source, host);
  Bool results_linked = results.link_types(source, host);
  return parameters_linked && results_linked;
}

auto Language::Signature::validate_publication(
    Tetrodotoxin::Language::Monograph& source,
    const Type& host) const -> Bool {
  Bool parameters_valid = parameters.validate_publication(source, host);
  Bool results_valid = results.validate_publication(source, host);
  return parameters_valid && results_valid;
}

auto Language::Signature::declares_self() const -> Bool {
  return parameters.declares_self();
}

auto Language::Signature::is_linked() const -> Bool {
  return parameters.is_linked() && results.is_linked();
}
