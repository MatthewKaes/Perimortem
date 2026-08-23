// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "backend/llvm/lowering/execution.hpp"

#include "backend/llvm/lowering/access.hpp"
#include "backend/llvm/lowering/control.hpp"
#include "backend/llvm/lowering/operations.hpp"
#include "backend/llvm/lowering/values.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Backend;

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Model::Pack& pack) const -> Bool {
  auto expression = pack.select<Tetrodotoxin::Library::Language::Expression>();
  if (expression) {
    auto folded = expression->get_folded();
    if (folded && &*folded != &pack) {
      return lower(*folded) && storage.alias(pack, *folded);
    }

    if (Values::lower(*this, *expression)) {
      return True;
    }
    if (Operations::lower(*this, *expression)) {
      return True;
    }
    return Access::lower(*this, *expression);
  }

  for (const Ttx::Concept::Reference<
           Tetrodotoxin::Library::Language::Model::Pack>& entry :
       pack.get_entries()) {
    if (!lower(entry.get())) {
      return False;
    }
  }
  return storage.compose(pack);
}

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Statement& statement) const -> Bool {
  return Control::lower(*this, statement);
}

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Flow::Block& block) const -> Bool {
  return Control::lower(*this, block);
}

auto Llvm::Lowering::Execution::lower_write_target(
    const Tetrodotoxin::Library::Language::Expression& expression) const
    -> Bool {
  return Access::lower_write_target(*this, expression);
}
