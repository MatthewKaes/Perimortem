// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/semantic.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Puffer;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;

auto Lsp::semantic_subject(const Abstract& semantic) -> const Abstract& {
  auto call = semantic.select<Access::Call>();
  if (call) {
    auto callable = call->get_callable();
    if (callable) {
      return *callable;
    }
  }

  auto expression = semantic.select<Expression>();
  if (expression) {
    const Abstract& result = expression->get_result();
    if (&result != &*expression && !result.is<Invalid>()) {
      return result;
    }
  }
  return semantic;
}
