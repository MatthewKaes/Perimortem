// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/compiler.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/serialization/escaped_text.hpp"

#include "tetrodotoxin/syntax/ast/statement.hpp"
#include "tetrodotoxin/syntax/expression/literal.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

namespace Tetrodotoxin::Compiler {
namespace {

auto is_view_bytes(Abi::Type type) -> Bool {
  return type.carrier == Abi::Type::Carrier::ViewBytes;
}

auto first_view_bytes_argument(
    View::Vector<Abi::Argument> arguments,
    View::Bytes argument_name) -> Count {
  for (Count i = 0; i < arguments.get_size(); i++) {
    if (arguments[i].name == argument_name && is_view_bytes(arguments[i].type)) {
      return i;
    }
  }

  return Count(-1);
}

auto is_view_literal(const Syntax::Expression::Expression* expression_value)
    -> Bool {
  return expression_value &&
         expression_value->get_kind() == Syntax::Expression::Kind::Literal &&
         (expression_value->get_class() == Lexical::Class::Type::String ||
          expression_value->get_class() == Lexical::Class::Type::Bytes);
}

auto can_load_view_value(
    View::Vector<Abi::Argument> arguments,
    const Syntax::Expression::Expression* argument_expression) -> Bool {
  if (is_view_literal(argument_expression)) {
    return True;
  }

  return argument_expression &&
         argument_expression->get_kind() ==
             Syntax::Expression::Kind::Addressable &&
         first_view_bytes_argument(arguments, argument_expression->get_text()) ==
             0;
}

auto can_lower_library_body(
    View::Vector<Abi::Argument> arguments,
    View::Vector<Syntax::Ast::Statement*> function_body) -> Bool {
  if (arguments.get_size() > 1) {
    return False;
  }

  if (arguments.get_size() == 1 && !is_view_bytes(arguments[0].type)) {
    return False;
  }

  for (Count i = 0; i < function_body.get_size(); i++) {
    const Syntax::Ast::Statement* body_statement = function_body[i];
    if (!body_statement) {
      continue;
    }

    if (body_statement->get_class() == Lexical::Class::Type::Return) {
      return !body_statement->get_value();
    }

    if (body_statement->get_class() != Lexical::Class::Type::EndStatement ||
        !body_statement->get_value()) {
      return False;
    }

    const Syntax::Expression::Expression* call_expression =
        body_statement->get_value();
    if (call_expression->get_kind() != Syntax::Expression::Kind::Call ||
        !call_expression->get_left() ||
        call_expression->get_left()->get_kind() !=
            Syntax::Expression::Kind::TypeAccess) {
      return False;
    }

    const Syntax::Expression::Expression* pack_expression =
        call_expression->get_right();
    if (!pack_expression ||
        pack_expression->get_kind() != Syntax::Expression::Kind::Pack) {
      return False;
    }

    const auto* call_pack = static_cast<const Syntax::Pack*>(pack_expression);
    View::Vector<Syntax::Pack::Field> call_fields = call_pack->get_fields();
    if (call_fields.get_size() != 1 || !call_fields[0].name.is_empty() ||
        !call_fields[0].index.is_empty() || !call_fields[0].value ||
        !can_load_view_value(arguments, call_fields[0].value)) {
      return False;
    }
  }

  return True;
}

}  // namespace

auto Compiler::compile_function(
    View::Bytes module_name,
    View::Bytes function_name,
    Abi::Type return_type,
    View::Vector<Abi::Argument> arguments,
    View::Vector<Syntax::Ast::Statement*> function_body) -> Bool {
  if (!can_lower_library_body(arguments, function_body)) {
    return False;
  }

  Assembler::x86_64 assembler(machine_code);

  const auto canonical_function_name =
      names.canonicalize(module_name, function_name);
  const Count function_code_start = machine_code.get_size();

  assembler.prologue(arguments);

  // Current Library lowering supports one host-visible View[Bytes] argument.
  // It is copied into callee-saved registers before calls can clobber the
  // System V argument registers.
  Bool saved_view_argument = False;
  if (arguments.get_size() == 1) {
    assembler.push(Assembler::x86_64::Reg::RBX);
    assembler.push(Assembler::x86_64::Reg::R12);
    assembler.mov(Assembler::x86_64::Reg::RDI, Assembler::x86_64::Reg::RBX);
    assembler.mov(Assembler::x86_64::Reg::RSI, Assembler::x86_64::Reg::R12);
    saved_view_argument = True;
  }

  for (Count i = 0; i < function_body.get_size(); i++) {
    const Syntax::Ast::Statement* body_statement = function_body[i];
    if (!body_statement) {
      continue;
    }

    if (body_statement->get_class() == Lexical::Class::Type::Return) {
      break;
    }

    const Syntax::Expression::Expression* call_expression =
        body_statement->get_value();
    const auto* call_pack =
        static_cast<const Syntax::Pack*>(call_expression->get_right());
    View::Vector<Syntax::Pack::Field> call_fields = call_pack->get_fields();
    const Syntax::Expression::Expression* field_value = call_fields[0].value;
    if (is_view_literal(field_value)) {
      View::Bytes literal_text =
          Syntax::Expression::Literal::inner_text(field_value->get_text());
      literal_text = EscapedText::decode(arena, literal_text);
      load_string(assembler, Assembler::x86_64::Reg::RDI, literal_text);
      assembler.mov(literal_text.get_size(), Assembler::x86_64::Reg::RSI);
    } else if (
        field_value->get_kind() == Syntax::Expression::Kind::Addressable &&
        first_view_bytes_argument(arguments, field_value->get_text()) == 0) {
      assembler.mov(Assembler::x86_64::Reg::RBX, Assembler::x86_64::Reg::RDI);
      assembler.mov(Assembler::x86_64::Reg::R12, Assembler::x86_64::Reg::RSI);
    }

    call_extern(assembler, call_expression->get_text());
  }

  if (saved_view_argument) {
    assembler.pop(Assembler::x86_64::Reg::R12);
    assembler.pop(Assembler::x86_64::Reg::RBX);
  }
  assembler.epilogue(return_type);

  const Count function_code_size =
      machine_code.get_size() - function_code_start;

  auto function_symbol = Context::Symbol::create_func(
      canonical_function_name, Context::Symbol::Visability::Global);
  function_symbol.set_range({function_code_start, function_code_size});

  const Count symbol_index = symbols.get_size();
  symbols.insert(function_symbol);

  // Copy arguments into arena so the function entry remains valid after
  // the caller's stack frame is gone.
  auto* arguments_copy = Data::cast<Abi::Argument>(
      arena.allocate(sizeof(Abi::Argument) * arguments.get_size()));
  for (Count j = 0; j < arguments.get_size(); j++) {
    arguments_copy[j] = arguments.at(j);
  }

  functions.insert({
    canonical_function_name,
    View::Vector<Abi::Argument>(arguments_copy, arguments.get_size()),
    return_type,
    symbol_index,
  });

  return True;
}

auto Compiler::add_read_only_data(
    View::Bytes name,
    View::Bytes data,
    Count alignment,
    Context::Symbol::Visability visability) -> Count {
  if (alignment == 0) {
    alignment = 1;
  }

  while (read_only.get_size() % alignment != 0) {
    read_only.append('\0');
  }

  const Count start = read_only.get_size();
  read_only.concat(data);

  const Count symbol_index = symbols.get_size();
  symbols.insert(
      Context::Symbol::create_read_only(
          names.retain(name), {start, data.get_size()}, visability));
  return symbol_index;
}

auto Compiler::generate_cpp_header(Dynamic::Bytes& header_out) -> void {
  // Generated header
  header_out.concat(
      "//    This file is generated by the Tetrodotoxin compiler\n"_view);
  header_out.concat("//         Manual edits can break the C++/TTX ABI\n\n"_view);
  header_out.concat("#pragma once\n\n"_view);

  // TODO: Sort out which headers are actually required.
  header_out.concat("#include \"perimortem/core/view/bytes.hpp\"\n\n"_view);

  header_out.concat("namespace Ttx {\n\n"_view);
  header_out.concat("extern \"C\" {\n\n"_view);

  // Write out all of the exported functions.
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto& compiled_function = functions.at(i);
    header_out.concat(compiled_function.return_type.cpp_name);
    header_out.concat(" "_view);
    header_out.concat(compiled_function.name);
    header_out.append('(');
    for (Count j = 0; j < compiled_function.arguments.get_size(); j++) {
      if (j > 0) {
        header_out.concat(", "_view);
      }
      header_out.concat(compiled_function.arguments.at(j).type.cpp_name);
      header_out.append(' ');
      header_out.concat(compiled_function.arguments.at(j).name);
    }
    header_out.concat(");\n"_view);
  }

  header_out.concat("\n}  // extern \"C\"\n\n"_view);
  header_out.concat("}  // namespace Ttx\n"_view);
}

auto Compiler::load_string(
    Assembler::x86_64& assembler,
    Assembler::x86_64::Reg destination,
    View::Bytes string) -> void {
  const Count symbol_index = ref_string(string);
  assembler.read_only(destination);
  // read_only creates an LEA with a disp32 to be relocated
  relocs.insert(
      Context::Relocation::create_pc32(symbol_index, machine_code.get_size()));
}

auto Compiler::call_extern(
    Assembler::x86_64& assembler,
    View::Bytes function_name) -> void {
  const Count symbol_index =
      ref_extern(function_name, Context::Symbol::Type::Func);
  assembler.call();
  // call() creates a CALL with a disp32 to be relocated.
  relocs.insert(
      Context::Relocation::create_pc32(symbol_index, machine_code.get_size()));
}

auto Compiler::ref_extern(View::Bytes name, Context::Symbol::Type type)
    -> Count {
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.at(i).get_context() == Context::Symbol::Location::External &&
        symbols.at(i).get_name() == name) {
      return i;
    }
  }
  const Count index = symbols.get_size();
  symbols.insert(Context::Symbol::create_extrenal(name, type));
  return index;
}

auto Compiler::ref_string(View::Bytes string_value) -> Count {
  for (Count i = 0; i < symbols.get_size(); i++) {
    const auto& symbol = symbols.at(i);
    if (symbol.get_context() != Context::Symbol::Location::Strings) {
      continue;
    }
    const auto string_range = symbol.get_range();
    if (strings.get_view().slice(string_range.start, string_range.size) ==
        string_value) {
      return i;
    }
  }
  const Count symbol_index = symbols.get_size();
  const auto string_range = strings.add_string(string_value, symbol_index);
  symbols.insert(
      Context::Symbol::create_string(names.make_local_unique(), string_range));
  return symbol_index;
}

}  // namespace Tetrodotoxin::Compiler
