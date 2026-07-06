// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/library.hpp"

#include "perimortem/core/hash.hpp"

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "tetrodotoxin/isa/library/block.hpp"
#include "ttx/layout.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

auto Library::lower(Context& context, View::Bytes module, const Ttx::Type& root)
    -> Bool {
  View::Vector<Ttx::Type::Function> functions = root.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    if (!lower_function(context, module, functions[i])) {
      return False;
    }
  }
  return True;
}

auto Library::lower_function(
    Context& context,
    View::Bytes module,
    const Ttx::Type::Function& function) -> Bool {
  if (!function.has_body()) {
    return True;
  }

  if (!is_void_result(function.get_result())) {
    return context.report(
        "Only void Library functions can lower to C++ today."_view);
  }

  View::Vector<Ttx::Type::Member> parameters = function.get_parameters();
  if (parameters.get_size() > 1) {
    return context.report(
        "Only one View[Bytes] function parameter can lower today."_view);
  }

  if (parameters.get_size() == 1 && !is_view_bytes(parameters[0].get_type())) {
    return context.report(
        "Only View[Bytes] function parameters can lower today."_view);
  }

  Assembler::x86_64 assembler(machine_code);
  const Count function_start = machine_code.get_size();
  assembler.push(Assembler::x86_64::Reg::RBX);
  assembler.push(Assembler::x86_64::Reg::R12);
  assembler.push(Assembler::x86_64::Reg::RBP);

  if (parameters.get_size() == 1) {
    assembler.mov(Assembler::x86_64::Reg::RDI, Assembler::x86_64::Reg::RBX);
    assembler.mov(Assembler::x86_64::Reg::RSI, Assembler::x86_64::Reg::R12);
  }

  View::Vector<Ttx::Type::Function::Block> blocks = function.get_blocks();
  for (Count i = 0; i < blocks.get_size(); i++) {
    if (!lower_block(context, function, blocks[i])) {
      return False;
    }
  }

  assembler.pop(Assembler::x86_64::Reg::RBP);
  assembler.pop(Assembler::x86_64::Reg::R12);
  assembler.pop(Assembler::x86_64::Reg::RBX);
  assembler.ret();

  functions.insert({
    function_name(context, module, function.get_name()),
    {function_start, machine_code.get_size() - function_start},
    function,
  });
  return True;
}

auto Library::lower_block(
    Context& context,
    const Ttx::Type::Function& function,
    Ttx::Type::Function::Block block) -> Bool {
  const auto* library_block = Tetrodotoxin::Isa::Library::Block::from(block);
  if (library_block == nullptr) {
    return context.report("Function body is not a Library block."_view);
  }

  View::Vector<Tetrodotoxin::Isa::Library::Statement> statements =
      library_block->get_statements();
  for (Count i = 0; i < statements.get_size(); i++) {
    if (statements[i].get_kind() ==
        Tetrodotoxin::Isa::Library::Statement::Kind::Return) {
      return True;
    }

    if (!lower_statement(context, function, statements[i])) {
      return False;
    }
  }
  return True;
}

auto Library::lower_statement(
    Context& context,
    const Ttx::Type::Function& function,
    const Tetrodotoxin::Isa::Library::Statement& statement) -> Bool {
  switch (statement.get_kind()) {
  case Tetrodotoxin::Isa::Library::Statement::Kind::Call:
    return lower_call(context, function, statement.get_call());
  default:
    return context.report("Unsupported Library statement."_view);
  }
}

auto Library::lower_call(
    Context& context,
    const Ttx::Type::Function& function,
    const Tetrodotoxin::Isa::Library::Call& call) -> Bool {
  const Ttx::Type* owner = call.get_owner();
  if (owner == nullptr ||
      !owner->attribute_equals("isa"_view, "Foreign"_view)) {
    return context.report("Call target is not a foreign type."_view);
  }

  const Ttx::Type::Function* callee = call.get_function();
  if (callee == nullptr) {
    return context.report("Foreign function could not be resolved."_view);
  }

  View::Vector<Tetrodotoxin::Isa::Expression::Value> values =
      call.get_pack().get_values();
  if (values.get_size() != 1) {
    return context.report(
        "Only foreign calls with one View[Bytes] pack value can lower today."_view);
  }

  if (!lower_pack_value(context, function, values[0], *callee)) {
    return False;
  }

  call_external(call.get_name());
  return True;
}

auto Library::lower_pack_value(
    Context& context,
    const Ttx::Type::Function& function,
    const Tetrodotoxin::Isa::Expression::Value& value,
    const Ttx::Type::Function& callee) -> Bool {
  View::Vector<Ttx::Type::Member> parameters = callee.get_parameters();
  if (parameters.get_size() != 1 || !is_view_bytes(parameters[0].get_type())) {
    return context.report(
        "Only foreign calls with one View[Bytes] pack value can lower today."_view);
  }

  switch (value.get_kind()) {
  case Tetrodotoxin::Isa::Expression::Value::Kind::String:
    return emit_string_argument(context, value.get_value());
  case Tetrodotoxin::Isa::Expression::Value::Kind::Reference:
    return emit_parameter_argument(context, function, value.get_value());
  default:
    return context.report("Unsupported foreign call pack value."_view);
  }
}

auto Library::emit_string_argument(Context& context, View::Bytes value)
    -> Bool {
  const Count target_index = string_index(context, value);

  Assembler::x86_64 assembler(machine_code);
  assembler.read_only(Assembler::x86_64::Reg::RDI);
  relocations.insert({
    Symbol::Relocation::Target::String,
    target_index,
    machine_code.get_size(),
    Symbol::Relocation::Type::Pc32,
  });
  assembler.mov(Bits_64(value.get_size()), Assembler::x86_64::Reg::RSI);
  return True;
}

auto Library::emit_parameter_argument(
    Context& context,
    const Ttx::Type::Function& function,
    View::Bytes name) -> Bool {
  const Ttx::Type::Member* parameter =
      Ttx::Layout(function.get_parameters()).find_member(name);
  if (parameter == nullptr) {
    return context.report(
        "Foreign call pack value did not resolve to a parameter."_view);
  }

  if (!is_view_bytes(parameter->get_type())) {
    return context.report("Only View[Bytes] parameters can lower today."_view);
  }

  Assembler::x86_64 assembler(machine_code);
  assembler.mov(Assembler::x86_64::Reg::RBX, Assembler::x86_64::Reg::RDI);
  assembler.mov(Assembler::x86_64::Reg::R12, Assembler::x86_64::Reg::RSI);
  return True;
}

auto Library::call_external(View::Bytes name) -> void {
  const Count target_index = external_index(name);
  Assembler::x86_64 assembler(machine_code);
  assembler.call();
  relocations.insert({
    Symbol::Relocation::Target::External,
    target_index,
    machine_code.get_size(),
    Symbol::Relocation::Type::Plt32,
  });
}

auto Library::string_index(Context& context, View::Bytes value) -> Count {
  for (Count i = 0; i < strings.get_size(); i++) {
    if (strings[i].value == value) {
      return i;
    }
  }

  const Count offset = string_data.get_size();
  string_data.concat(value);
  const Count index = strings.get_size();
  strings.insert(
      {local_string_name(context, value), value, {offset, value.get_size()}});
  return index;
}

auto Library::external_index(View::Bytes name) -> Count {
  for (Count i = 0; i < externals.get_size(); i++) {
    if (externals[i].name == name) {
      return i;
    }
  }

  const Count index = externals.get_size();
  externals.insert({name});
  return index;
}

auto Library::function_name(
    Context& context,
    View::Bytes module,
    View::Bytes function) -> View::Bytes {
  Managed::Bytes output(context.get_arena());
  output.concat("TTX_"_view);
  append_name_segment(output, module);
  output.append('_');
  append_name_segment(output, function);
  return output;
}

auto Library::local_string_name(Context& context, View::Bytes value)
    -> View::Bytes {
  Managed::Bytes output(context.get_arena());
  output.concat(".Lttx_"_view);
  append_hex(output, Hash(value).get_value());
  output.append('_');
  append_hex(output, string_data.get_size());
  return output;
}

auto Library::append_name_segment(Managed::Bytes& output, View::Bytes value)
    -> void {
  for (Count i = 0; i < value.get_size(); i++) {
    Bits_8 c = value[i];
    const Bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const Bool digit = c >= '0' && c <= '9';
    output.append(alpha || digit || c == '_' ? c : Bits_8('_'));
  }
}

auto Library::append_hex(Managed::Bytes& output, Bits_64 value) -> void {
  constexpr View::Bytes digits = "0123456789abcdef"_view;
  for (Signed_32 shift = 60; shift >= 0; shift -= 4) {
    output.append(digits[(value >> shift) & 0x0F]);
  }
}

auto Library::is_view_bytes(const Ttx::Type* type) -> Bool {
  return type != nullptr &&
         type->attribute_equals("abi"_view, "view_bytes"_view);
}

auto Library::is_void_result(View::Vector<Ttx::Type::Member> result) -> Bool {
  if (result.is_empty()) {
    return True;
  }

  const Ttx::Type* type = result[0].get_type();
  return result.get_size() == 1 && type != nullptr &&
         type->attribute_equals("abi"_view, "void"_view);
}
