// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/contract.hpp"

#include "ttx/layout.hpp"

using namespace Perimortem::Core;
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

  return validate_reads(cursor, contract, function);
}

auto Shader::Contract::validate_reads(
    Cursor& cursor,
    const Ttx::Type& contract,
    const Ttx::Type::Function& function) -> Bool {
  const Ttx::Type* stage_facts = contract.find_type(function.get_name());
  if (stage_facts == nullptr) {
    cursor.token_error("Shader stage has no render fact contract."_view);
    return False;
  }

  View::Vector<Ttx::Type::Function::Block> blocks = function.get_blocks();
  for (Count i = 0; i < blocks.get_size(); i++) {
    View::Vector<Token> tokens = blocks[i].get_tokens();
    for (Count j = 0; j < tokens.get_size(); j++) {
      const Token& root = tokens[j];
      if (root.get_class() != Class::Type::Addressable ||
          !is_read_root(root.get_text())) {
        continue;
      }

      if (j + 1 >= tokens.get_size() ||
          tokens[j + 1].get_class() != Class::Type::AddressOp) {
        continue;
      }

      if (j + 2 >= tokens.get_size() ||
          tokens[j + 2].get_class() != Class::Type::Addressable) {
        cursor.range_error(
            root, root, "Shader render fact access needs a member name."_view);
        return False;
      }

      const Ttx::Type* reads = stage_facts->find_type(root.get_text());
      if (reads == nullptr ||
          reads->find_member(tokens[j + 2].get_text()) == nullptr) {
        cursor.range_error(
            tokens[j + 2], tokens[j + 2],
            "Shader stage cannot read render fact."_view);
        return False;
      }
    }
  }

  return True;
}
