// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

#include "tetrodotoxin/library/language/fold.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/access.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/control.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/operations.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/values.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Model::Pack& pack) const -> Bool {
  auto identity = pack.get_identity();
  auto constant =
      identity
          ? identity->select<Tetrodotoxin::Library::Language::Constant>()
          : Core::Option<
                const Tetrodotoxin::Library::Language::Constant&>();
  if (constant) {
    return Values::lower(*this, *constant);
  }

  auto expression =
      pack.select_identity<Tetrodotoxin::Library::Language::Expression>();
  if (expression) {
    auto folded = Tetrodotoxin::Library::Language::query_folded_pack(pack);
    if (folded && &*folded != &pack) {
      auto folded_domain =
          folded->get_type().resolve().select<Ttx::Model::Domain>();
      auto result_domain =
          pack.get_type().resolve().select<Ttx::Model::Domain>();
      if (folded_domain && result_domain &&
          &*folded_domain == &*result_domain) {
        // Fitting proves that folded inputs can participate in an operation;
        // it does not prove that their Pack is the operation's result. Reusing
        // the native value is valid only when both sides publish the same
        // Domain. Aggregate initialization otherwise still constructs its
        // one result from the folded child values.
        return lower(*folded) && storage.alias(pack, *folded);
      }
    }

    if (Operations::lower(*this, *expression)) {
      return True;
    }
    return Access::lower(*this, *expression);
  }

  for (const Tetrodotoxin::Library::Language::Model::Pack* entry :
       pack.get_entries()) {
    if (!lower(*entry)) {
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
