// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/signature.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Signature::interpret(Cursor& cursor, const Abstract& host)
    -> Option<Signature&> {
  Allocator::Arena& domain = cursor.get_arena();

  auto parameters = Language::Model::Layout::interpret_parameters(cursor, host);
  BAIL_IF(!parameters);

  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Library Function parameters require `->` before the result "
      "Layout."_view));

  auto results = Language::Model::Layout::interpret(cursor, host);
  BAIL_IF(!results);

  Signature& signature = domain.construct_from<Signature>(
      [&]() -> Signature { return Signature(host, *parameters, *results); });
  return signature;
}

auto Language::Signature::link(Cursor& cursor) -> Bool {
  // Both models run so one malformed parameter cannot hide an independent
  // result diagnostic. Each model owns idempotence for its exact staged edges.
  Bool parameters_linked = parameters.link_parameters(cursor, host);
  Bool results_linked = results.link_types(cursor, host);
  return parameters_linked && results_linked;
}

auto Language::Signature::validate_publication(Cursor& cursor) const -> Bool {
  Bool parameters_valid = parameters.validate_publication(cursor, host);
  Bool results_valid = results.validate_publication(cursor, host);
  return parameters_valid && results_valid;
}

auto Language::Signature::declares_self() const -> Bool {
  return parameters.declares_self();
}

auto Language::Signature::is_linked() const -> Bool {
  return parameters.is_linked() && results.is_linked();
}
