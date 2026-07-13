// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/contract.hpp"

#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"
#include "ttx/layout.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static constexpr auto is_read_root(View::Bytes name) -> Bool {
  return name == "constant"_view || name == "push"_view ||
         name == "resource"_view;
}

static auto validate_reference_read(
    Cursor& cursor,
    const Ttx::Type& stage_facts,
    View::Vector<Token> tokens) -> Bool {
  for (Count i = 0; i < tokens.get_size(); i++) {
    const Token& root = tokens[i];
    if (root.get_class() != Class::Type::Addressable ||
        !is_read_root(root.get_text())) {
      continue;
    }

    if (i + 1 >= tokens.get_size() ||
        tokens[i + 1].get_class() != Class::Type::AddressOp) {
      cursor.range_error(
          root, root, "Shader render fact access needs a member name."_view);
      return False;
    }

    if (i + 2 >= tokens.get_size() ||
        tokens[i + 2].get_class() != Class::Type::Addressable) {
      cursor.range_error(
          root, root, "Shader render fact access needs a member name."_view);
      return False;
    }

    const Ttx::Type* reads = stage_facts.find_type(root.get_text());
    if (reads == nullptr ||
        reads->find_member(tokens[i + 2].get_text()) == nullptr) {
      cursor.range_error(
          tokens[i + 2], tokens[i + 2],
          "Shader stage cannot read render fact."_view);
      return False;
    }
  }

  return True;
}

static auto validate_pack_reads(
    Cursor& cursor,
    const Ttx::Type& stage_facts,
    const Base::Expression::Pack* pack) -> Bool;

static auto validate_value_reads(
    Cursor& cursor,
    const Ttx::Type& stage_facts,
    Base::Expression::Value value) -> Bool {
  switch (value.get_kind()) {
  case Base::Expression::Value::Kind::Reference:
    return validate_reference_read(cursor, stage_facts, value.get_tokens());
  case Base::Expression::Value::Kind::Pack:
    return validate_pack_reads(cursor, stage_facts, value.get_pack());
  case Base::Expression::Value::Kind::Binary:
    return validate_value_reads(cursor, stage_facts, *value.get_left()) &&
           validate_value_reads(cursor, stage_facts, *value.get_right());
  default:
    return True;
  }
}

static auto validate_pack_reads(
    Cursor& cursor,
    const Ttx::Type& stage_facts,
    const Base::Expression::Pack* pack) -> Bool {
  if (pack == nullptr) {
    return True;
  }

  View::Vector<Base::Expression::Value> values = pack->get_values();
  for (Count i = 0; i < values.get_size(); i++) {
    if (!validate_value_reads(cursor, stage_facts, values[i])) {
      return False;
    }
  }

  return True;
}

static auto validate_statement_reads(
    Cursor& cursor,
    const Ttx::Type& stage_facts,
    const Shader::Statement& statement) -> Bool {
  const Shader::Statement::State* state = statement.find_state();
  if (state != nullptr) {
    return validate_value_reads(cursor, stage_facts, state->get_initializer());
  }

  return statement.is_return()
             ? validate_pack_reads(
                   cursor, stage_facts, statement.get_return_pack())
             : True;
}

auto Shader::Contract::resolve(Cursor& cursor, Base::Context& context)
    -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` before shader render contract."_view)) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* contract = Base::Expression::Type::evaluate(cursor, context);
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
    const Ttx::Function& function,
    const Shader::Block& block) -> Bool {
  const Ttx::Function* stage = contract.find_function(function.get_name());
  if (stage == nullptr) {
    cursor.token_error(
        "Shader stage is not declared by the render contract."_view);
    return False;
  }

  if (!function.get_parameters().fits(stage->get_parameters())) {
    cursor.token_error(
        "Shader stage parameters do not match the render contract."_view);
    return False;
  }

  if (!function.get_result().fits(stage->get_result())) {
    cursor.token_error(
        "Shader stage result does not match the render contract."_view);
    return False;
  }

  return validate_reads(cursor, contract, function, block);
}

auto Shader::Contract::validate_reads(
    Cursor& cursor,
    const Ttx::Type& contract,
    const Ttx::Function& function,
    const Shader::Block& block) -> Bool {
  const Ttx::Type* stage_facts = contract.find_type(function.get_name());
  if (stage_facts == nullptr) {
    cursor.token_error("Shader stage has no render fact contract."_view);
    return False;
  }

  View::Vector<Shader::Statement> statements = block.get_statements();
  for (Count i = 0; i < statements.get_size(); i++) {
    if (!validate_statement_reads(cursor, *stage_facts, statements[i])) {
      return False;
    }
  }

  return True;
}
