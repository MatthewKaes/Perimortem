// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/compiler.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/serialization/escaped_text.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

namespace Tetrodotoxin::Compiler {
namespace Helpers {

auto is_view_bytes(Abi::Type type) -> Bool {
  return type.carrier == Abi::Type::Carrier::ViewBytes;
}

auto can_lower_library_signature(View::Vector<Abi::Argument> arguments)
    -> Bool {
  if (arguments.get_size() > 1) {
    return False;
  }

  return arguments.is_empty() || is_view_bytes(arguments[0].type);
}

auto align_runtime(Count offset, Count alignment) -> Count {
  if (alignment <= 1) {
    return offset;
  }

  const Count remainder = offset % alignment;
  return remainder == 0 ? offset : offset + (alignment - remainder);
}

auto foreign_call_object_stack_size(const Abi::ForeignCall& foreign_call)
    -> Count {
  Count result = 0;
  View::Vector<Abi::Value> arguments = foreign_call.get_arguments();
  for (Count argument_index = 0; argument_index < arguments.get_size();
       argument_index++) {
    if (!arguments[argument_index].is_object()) {
      continue;
    }

    const Abi::ObjectDefinition& definition =
        arguments[argument_index].get_object().get_definition();
    result = align_runtime(result, definition.get_alignment());
    result += definition.get_byte_size();
  }

  return align_runtime(result, 16);
}

auto object_stack_size(View::Vector<Abi::ForeignCall> foreign_calls) -> Count {
  Count result = 0;
  for (Count i = 0; i < foreign_calls.get_size(); i++) {
    Count call_stack_size = foreign_call_object_stack_size(foreign_calls[i]);
    if (call_stack_size > result) {
      result = call_stack_size;
    }
  }

  return result;
}

auto real_32_bits(Real_32 value) -> Bits_32 {
  Bits_32 bits = 0;
  Data::copy(Data::cast<Bits_8>(&bits), &value, 1);
  return bits;
}

auto real_64_bits(Real_64 value) -> Bits_64 {
  Bits_64 bits = 0;
  Data::copy(Data::cast<Bits_8>(&bits), &value, 1);
  return bits;
}

auto append_count(Dynamic::Bytes& output, Count value) -> void {
  Static::Bytes<32> text;
  Writer::Textual writer(text.get_access());
  writer << value;
  output.concat(writer);
}

}  // namespace Helpers

auto Compiler::add_object_definition(const Abi::ObjectDefinition& definition)
    -> void {
  if (!definition.is_valid()) {
    return;
  }

  for (Count i = 0; i < object_definitions.get_size(); i++) {
    if (object_definitions[i].get_cpp_name() == definition.get_cpp_name()) {
      return;
    }
  }

  Abi::ObjectDefinition retained_definition(
      names.retain(definition.get_type_name()),
      names.retain(definition.get_cpp_name()));
  retained_definition.set_storage(
      definition.get_byte_size(), definition.get_alignment());

  View::Vector<Abi::ObjectField> fields = definition.get_fields();
  for (Count i = 0; i < fields.get_size(); i++) {
    retained_definition.add_field({
      names.retain(fields[i].get_name()),
      names.retain(fields[i].get_type_name()),
      names.retain(fields[i].get_cpp_type_name()),
      fields[i].get_offset(),
      fields[i].get_byte_size(),
      fields[i].get_alignment(),
    });
  }

  object_definitions.insert(retained_definition);
}

auto Compiler::store_object_value(
    Assembler::x86_64& assembler,
    const Abi::ObjectField& field,
    const Abi::ObjectValue& value,
    Signed_32 displacement) -> void {
  using Reg = Assembler::x86_64::Reg;

  switch (value.get_source()) {
  case Abi::ObjectValue::Source::Zeroed:
    store_zeroed_bytes(assembler, field.get_byte_size(), displacement);
    return;

  case Abi::ObjectValue::Source::Object: {
    const Abi::Object& nested_object = value.get_object();
    if (nested_object.get_definition().get_byte_size() != field.get_byte_size()) {
      return;
    }

    store_object(assembler, nested_object, displacement);
    return;
  }

  case Abi::ObjectValue::Source::UnsignedInteger:
    switch (field.get_byte_size()) {
    case 1:
      assembler.mov(Bits_8(value.get_unsigned_integer()), Reg::AL);
      assembler.mov(Reg::AL, Reg::RBP, displacement);
      return;
    case 2:
      assembler.mov(Bits_16(value.get_unsigned_integer()), Reg::AX);
      assembler.mov(Reg::AX, Reg::RBP, displacement);
      return;
    case 4:
      assembler.mov(Bits_32(value.get_unsigned_integer()), Reg::EAX);
      assembler.mov(Reg::EAX, Reg::RBP, displacement);
      return;
    case 8:
      assembler.mov(Bits_64(value.get_unsigned_integer()), Reg::RAX);
      assembler.mov(Reg::RAX, Reg::RBP, displacement);
      return;
    }
    break;

  case Abi::ObjectValue::Source::SignedInteger:
    switch (field.get_byte_size()) {
    case 1:
      assembler.mov(Bits_8(value.get_signed_integer()), Reg::AL);
      assembler.mov(Reg::AL, Reg::RBP, displacement);
      return;
    case 2:
      assembler.mov(Bits_16(value.get_signed_integer()), Reg::AX);
      assembler.mov(Reg::AX, Reg::RBP, displacement);
      return;
    case 4:
      assembler.mov(Bits_32(value.get_signed_integer()), Reg::EAX);
      assembler.mov(Reg::EAX, Reg::RBP, displacement);
      return;
    case 8:
      assembler.mov(Bits_64(value.get_signed_integer()), Reg::RAX);
      assembler.mov(Reg::RAX, Reg::RBP, displacement);
      return;
    }
    break;

  case Abi::ObjectValue::Source::Real32:
    assembler.mov(Helpers::real_32_bits(value.get_real_32()), Reg::EAX);
    assembler.mov(Reg::EAX, Reg::RBP, displacement);
    return;

  case Abi::ObjectValue::Source::Real64:
    assembler.mov(Helpers::real_64_bits(value.get_real_64()), Reg::RAX);
    assembler.mov(Reg::RAX, Reg::RBP, displacement);
    return;

  case Abi::ObjectValue::Source::ViewBytesLiteral: {
    View::Bytes literal_text = value.get_literal_payload();
    literal_text = EscapedText::decode(arena, literal_text);
    load_string(assembler, Reg::RAX, literal_text);
    assembler.mov(Reg::RAX, Reg::RBP, displacement);
    assembler.mov(Bits_64(literal_text.get_size()), Reg::RAX);
    assembler.mov(Reg::RAX, Reg::RBP, displacement + Signed_32(sizeof(Count)));
    return;
  }

  case Abi::ObjectValue::Source::RenderImport: {
    View::Bytes import_name = value.get_render_import_name();
    load_string(assembler, Reg::RAX, import_name);
    assembler.mov(Reg::RAX, Reg::RBP, displacement);
    assembler.mov(Bits_64(import_name.get_size()), Reg::RAX);
    assembler.mov(Reg::RAX, Reg::RBP, displacement + Signed_32(sizeof(Count)));
    return;
  }

  case Abi::ObjectValue::Source::Invalid:
    break;
  }
}

auto Compiler::store_zeroed_bytes(
    Assembler::x86_64& assembler,
    Count byte_size,
    Signed_32 displacement) -> void {
  using Reg = Assembler::x86_64::Reg;

  assembler.zero(Reg::RAX);
  Count byte_offset = 0;
  while (byte_size >= 8) {
    assembler.mov(Reg::RAX, Reg::RBP, displacement + Signed_32(byte_offset));
    byte_offset += 8;
    byte_size -= 8;
  }

  if (byte_size >= 4) {
    assembler.mov(Reg::EAX, Reg::RBP, displacement + Signed_32(byte_offset));
    byte_offset += 4;
    byte_size -= 4;
  }

  if (byte_size >= 2) {
    assembler.mov(Reg::AX, Reg::RBP, displacement + Signed_32(byte_offset));
    byte_offset += 2;
    byte_size -= 2;
  }

  if (byte_size == 1) {
    assembler.mov(Reg::AL, Reg::RBP, displacement + Signed_32(byte_offset));
  }
}

auto Compiler::store_object(
    Assembler::x86_64& assembler,
    const Abi::Object& object,
    Signed_32 displacement) -> void {
  View::Vector<Abi::ObjectField> fields =
      object.get_definition().get_fields();
  View::Vector<Abi::ObjectValue> values = object.get_values();
  for (Count i = 0; i < fields.get_size(); i++) {
    store_object_value(
        assembler, fields[i], values[i],
        displacement + Signed_32(fields[i].get_offset()));
  }
}

auto Compiler::compile_func(
    View::Bytes module_name,
    View::Bytes function_name,
    Abi::Type return_type,
    View::Vector<Abi::Argument> arguments,
    View::Vector<Abi::ForeignCall> foreign_calls) -> Bool {
  if (!Helpers::can_lower_library_signature(arguments)) {
    return False;
  }

  Assembler::x86_64 assembler(machine_code);

  const auto canonical_function_name =
      names.canonicalize(module_name, function_name);
  const Count function_code_start = machine_code.get_size();

  assembler.prologue(arguments);
  const Count object_stack_byte_size = Helpers::object_stack_size(foreign_calls);
  if (object_stack_byte_size > 0) {
    assembler.sub(Bits_32(object_stack_byte_size), Assembler::x86_64::Reg::RSP);
  }

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

  using Reg = Assembler::x86_64::Reg;
  static constexpr Static::Vector<Reg, 6> integer_argument_registers = {
    Reg::RDI,
    Reg::RSI,
    Reg::RDX,
    Reg::RCX,
    Reg::R8,
    Reg::R9,
  };

  for (Count i = 0; i < foreign_calls.get_size(); i++) {
    const Abi::ForeignCall& foreign_call = foreign_calls[i];
    View::Vector<Abi::Value> call_arguments = foreign_call.get_arguments();
    Count register_index = 0;
    Count object_stack_offset = 0;
    for (Count argument_index = 0; argument_index < call_arguments.get_size();
         argument_index++) {
      const Abi::Value& argument = call_arguments[argument_index];
      if (argument.is_object()) {
        if (register_index >= integer_argument_registers.get_size()) {
          return False;
        }

        const Abi::Object& object_argument = argument.get_object();
        const Abi::ObjectDefinition& definition =
            object_argument.get_definition();
        object_stack_offset =
            Helpers::align_runtime(
                object_stack_offset, definition.get_alignment());
        const Signed_32 object_displacement =
            -Signed_32(object_stack_byte_size) +
            Signed_32(object_stack_offset);
        store_object(assembler, object_argument, object_displacement);
        assembler.lea(
            Reg::RBP, object_displacement,
            integer_argument_registers[register_index]);
        object_stack_offset += definition.get_byte_size();
        register_index++;
        continue;
      }

      if (!argument.is_view_bytes() ||
          register_index + 1 >= integer_argument_registers.get_size()) {
        return False;
      }

      if (argument.is_view_bytes_literal()) {
        View::Bytes literal_text = argument.get_literal_payload();
        literal_text = EscapedText::decode(arena, literal_text);
        load_string(
            assembler, integer_argument_registers[register_index],
            literal_text);
        assembler.mov(
            Bits_64(literal_text.get_size()),
            integer_argument_registers[register_index + 1]);
      } else if (
          argument.is_view_bytes_runtime() &&
          argument.get_runtime_value_index() == 0) {
        assembler.mov(
            Reg::RBX, integer_argument_registers[register_index]);
        assembler.mov(
            Reg::R12, integer_argument_registers[register_index + 1]);
      } else {
        return False;
      }

      register_index += 2;
    }

    call_extern(assembler, foreign_call.get_callee_name());
  }

  if (saved_view_argument) {
    assembler.pop(Assembler::x86_64::Reg::R12);
    assembler.pop(Assembler::x86_64::Reg::RBX);
  }
  assembler.epilogue(return_type);

  const Count function_code_size =
      machine_code.get_size() - function_code_start;

  auto func_symbol = Context::Symbol::create_func(
      canonical_function_name, Context::Symbol::Visability::Global);
  func_symbol.set_range({function_code_start, function_code_size});

  const Count symbol_index = symbols.get_size();
  symbols.insert(func_symbol);

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

  header_out.concat("#include <stddef.h>\n\n"_view);
  header_out.concat("#include \"perimortem/core/view/bytes.hpp\"\n\n"_view);

  header_out.concat("namespace Ttx {\n\n"_view);

  for (Count i = 0; i < object_definitions.get_size(); i++) {
    const Abi::ObjectDefinition& definition = object_definitions[i];
    header_out.concat("struct alignas("_view);
    Helpers::append_count(header_out, definition.get_alignment());
    header_out.concat(") "_view);
    header_out.concat(definition.get_cpp_name());
    header_out.concat(" {\n"_view);

    View::Vector<Abi::ObjectField> fields = definition.get_fields();
    for (Count field_index = 0; field_index < fields.get_size();
         field_index++) {
      header_out.concat("  "_view);
      header_out.concat(fields[field_index].get_cpp_type_name());
      header_out.append(' ');
      header_out.concat(fields[field_index].get_name());
      header_out.concat(";\n"_view);
    }

    header_out.concat("};\n"_view);
    header_out.concat("static_assert(sizeof("_view);
    header_out.concat(definition.get_cpp_name());
    header_out.concat(") == "_view);
    Helpers::append_count(header_out, definition.get_byte_size());
    header_out.concat(");\n"_view);
    header_out.concat("static_assert(alignof("_view);
    header_out.concat(definition.get_cpp_name());
    header_out.concat(") == "_view);
    Helpers::append_count(header_out, definition.get_alignment());
    header_out.concat(");\n"_view);

    for (Count field_index = 0; field_index < fields.get_size();
         field_index++) {
      header_out.concat("static_assert(offsetof("_view);
      header_out.concat(definition.get_cpp_name());
      header_out.concat(", "_view);
      header_out.concat(fields[field_index].get_name());
      header_out.concat(") == "_view);
      Helpers::append_count(header_out, fields[field_index].get_offset());
      header_out.concat(");\n"_view);
    }

    header_out.append('\n');
  }

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
    if (symbols.at(i).get_ctx() == Context::Symbol::Location::External &&
        symbols.at(i).get_name() == name) {
      return i;
    }
  }
  const Count symbol_index = symbols.get_size();
  symbols.insert(Context::Symbol::create_extrenal(name, type));
  return symbol_index;
}

auto Compiler::ref_string(View::Bytes string_value) -> Count {
  for (Count i = 0; i < symbols.get_size(); i++) {
    const auto& symbol = symbols.at(i);
    if (symbol.get_ctx() != Context::Symbol::Location::Strings) {
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
