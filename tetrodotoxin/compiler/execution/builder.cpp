// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/execution/builder.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Compiler;

Execution::Builder::Builder(
    Allocator::Arena& arena,
    const Ttx::Function& signature)
    : arena(arena),
      signature(signature),
      operations(arena),
      operands(arena),
      bindings(arena) {
  View::Vector<Ttx::Member> parameters =
      signature.get_parameters().get_members();
  for (Count i = 0; i < parameters.get_size(); i++) {
    bindings.insert(Binding(parameters[i].get_type(), 0));
  }
}

auto Execution::Builder::parameter(const Ttx::Member& member) const -> Operand {
  View::Vector<Ttx::Member> parameters =
      signature.get_parameters().get_members();
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (&parameters[i] == &member) {
      return Operand(Addressable(i));
    }
  }

  return Operand();
}

auto Execution::Builder::binding_type(const Operand& operand)
    -> const Ttx::Type* {
  const Addressable* addressable = operand.find<Addressable>();
  if (addressable == nullptr || addressable->get_id() >= bindings.get_size()) {
    return nullptr;
  }

  return &bindings[addressable->get_id()].get_type();
}

auto Execution::Builder::append_operands(View::Vector<Operand> source)
    -> Range {
  Range range = {operands.get_size(), source.get_size()};
  for (Count i = 0; i < source.get_size(); i++) {
    operands.insert(source[i]);
  }

  return range;
}

auto Execution::Builder::binary(
    Binary::Operator op,
    const Ttx::Type& operand_type,
    const Ttx::Type& result_type,
    Operand left,
    Operand right) -> Operand {
  if (terminated) {
    return Operand();
  }

  const Ttx::Type* left_type = binding_type(left);
  const Ttx::Type* right_type = binding_type(right);
  if ((left_type != nullptr && !left_type->equivalent_to(operand_type)) ||
      (right_type != nullptr && !right_type->equivalent_to(operand_type)) ||
      left.is_null() || right.is_null()) {
    return Operand();
  }

  Operand folded = fold_binary(op, left, right);
  if (!folded.is_null()) {
    return folded;
  }

  const Count operation = operations.get_size();
  const Addressable result(bindings.get_size());
  bindings.insert(Binding(result_type, operation + 1));
  operations.insert(Operation(Binary(op, operand_type, left, right, result)));
  return Operand(result);
}

auto Execution::Builder::fold_binary(
    Binary::Operator op,
    const Operand& left,
    const Operand& right) const -> Operand {
  const Constant* left_constant = left.find<Constant>();
  const Constant* right_constant = right.find<Constant>();
  if (left_constant == nullptr || right_constant == nullptr) {
    return Operand();
  }

  return left_constant->visit(
      []() -> Operand { return Operand(); },
      [right_constant, op](const auto& left_value) -> Operand {
        using Type = __remove_cvref(decltype(left_value));
        const Type* right_value = right_constant->find<Type>();
        if (right_value == nullptr) {
          return Operand();
        }

        if (op == Binary::Operator::Equal) {
          return constant(Constant(Bool(left_value == *right_value)));
        }

        if constexpr (__is_same(Type, Bool) || __is_same(Type, View::Bytes)) {
          return Operand();
        } else if constexpr (__is_same(Type, Signed_64)) {
          Type result;
          switch (op) {
          case Binary::Operator::Add:
            return __builtin_add_overflow(left_value, *right_value, &result)
                       ? Operand()
                       : constant(Constant(result));
          case Binary::Operator::Subtract:
            return __builtin_sub_overflow(left_value, *right_value, &result)
                       ? Operand()
                       : constant(Constant(result));
          case Binary::Operator::Multiply:
            return __builtin_mul_overflow(left_value, *right_value, &result)
                       ? Operand()
                       : constant(Constant(result));
          case Binary::Operator::Divide:
          case Binary::Operator::Remainder: {
            constexpr Signed_64 minimum = (-9223372036854775807LL - 1);
            if (*right_value == 0 ||
                (left_value == minimum && *right_value == Signed_64(-1))) {
              return Operand();
            }

            return constant(Constant(
                op == Binary::Operator::Divide ? left_value / *right_value
                                               : left_value % *right_value));
          }
          default:
            return Operand();
          }
        } else {
          switch (op) {
          case Binary::Operator::Add:
            return constant(Constant(left_value + *right_value));
          case Binary::Operator::Subtract:
            return constant(Constant(left_value - *right_value));
          case Binary::Operator::Multiply:
            return constant(Constant(left_value * *right_value));
          case Binary::Operator::Divide:
            if constexpr (__is_same(Type, Bits_64)) {
              return *right_value == 0
                         ? Operand()
                         : constant(Constant(left_value / *right_value));
            } else {
              return constant(Constant(left_value / *right_value));
            }
          case Binary::Operator::Remainder:
            if constexpr (__is_same(Type, Bits_64)) {
              return *right_value == 0
                         ? Operand()
                         : constant(Constant(left_value % *right_value));
            }

            return Operand();
          default:
            return Operand();
          }
        }
      });
}

auto Execution::Builder::call(
    const Abi::Linkage& linkage,
    View::Vector<Operand> arguments) -> Range {
  const Ttx::Function& signature = linkage.get_function();
  View::Vector<Ttx::Member> parameters =
      signature.get_parameters().get_members();
  if (terminated || arguments.get_size() != parameters.get_size()) {
    return {Count(-1), 0};
  }

  for (Count i = 0; i < arguments.get_size(); i++) {
    const Ttx::Type* type = binding_type(arguments[i]);
    if (arguments[i].is_null() ||
        (type != nullptr && !type->equivalent_to(parameters[i].get_type()))) {
      return {Count(-1), 0};
    }
  }

  const Count operation = operations.get_size();
  Ttx::Layout result = signature.get_result();
  Range result_bindings = {bindings.get_size(), result.get_member_count()};
  View::Vector<Ttx::Member> result_members = result.get_members();
  for (Count i = 0; i < result_members.get_size(); i++) {
    bindings.insert(Binding(result_members[i].get_type(), operation + 1));
  }

  operations.insert(Operation(Call(
      signature, linkage.get_symbol(), append_operands(arguments),
      result_bindings)));
  return result_bindings;
}

auto Execution::Builder::return_values(View::Vector<Operand> values) -> Bool {
  if (terminated) {
    return False;
  }

  Ttx::Layout result = signature.get_result();
  if (values.get_size() != result.get_member_count()) {
    return False;
  }

  for (Count i = 0; i < values.get_size(); i++) {
    const Ttx::Type* type = binding_type(values[i]);
    if (values[i].is_null() ||
        (type != nullptr &&
         !type->equivalent_to(result.member_at(i).get_type()))) {
      return False;
    }
  }

  operations.insert(Operation(Return(append_operands(values))));
  terminated = True;
  return True;
}

auto Execution::Builder::finish() -> const Body* {
  if (!terminated && !signature.get_result().is_empty()) {
    return nullptr;
  }

  if (!terminated) {
    Bool returned = return_values();
    if (!returned) {
      return nullptr;
    }
  }

  const Block& block = arena.construct<Block>(Range{0, operations.get_size()});
  return &arena.construct<Body>(
      View::Vector<Block>(&block, 1), operations.get_view(),
      operands.get_view(), bindings.get_view());
}
