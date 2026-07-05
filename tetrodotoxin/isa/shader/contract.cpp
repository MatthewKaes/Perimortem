// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/contract.hpp"

#include "ttx/layout.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Contract::resolve(Context& context, Cursor& cursor)
    -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` before shader render contract."_view)) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* contract = context.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  if (contract == nullptr) {
    cursor.token_error("Shader render contract could not be resolved."_view);
    return nullptr;
  }

  return contract;
}

auto Shader::Contract::validate_stage(
    Cursor& cursor,
    const Ttx::Type& contract,
    const Ttx::Type::Function& function) -> Bool {
  const Ttx::Type::Function* stage =
      contract.find_function(function.get_name());
  if (stage == nullptr) {
    cursor.token_error("Shader stage is not declared by the render contract."_view);
    return False;
  }

  if (!Ttx::Layout(function.get_parameters())
           .fits(Ttx::Layout(stage->get_parameters()))) {
    cursor.token_error(
        "Shader stage parameters do not match the render contract."_view);
    return False;
  }

  if (!Ttx::Layout(function.get_result())
           .fits(Ttx::Layout(stage->get_result()))) {
    cursor.token_error(
        "Shader stage result does not match the render contract."_view);
    return False;
  }

  return True;
}
