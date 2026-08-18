// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/statement.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Statement::lower(Llvm::Builder& body) const -> Bool {
  if (!body.statement(root.get(), anchor)) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering could not begin one Statement."_view);
    return False;
  }

  Bool lowered = operations.lower(root.get(), body);
  if (!lowered) {
    Perimortem::Core::Diagnostics::Log::Message<256> message(
        Perimortem::Core::Diagnostics::Log::Level::Error,
        Perimortem::Core::Diagnostics::Source());
    message << "Library LLVM lowering could not emit Statement root '"_view
            << root.get().get_name() << "'."_view;
    return False;
  }

  Bool ended = body.end_statement(root.get());
  if (!ended) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering could not end one Statement."_view);
  }
  return ended;
}
