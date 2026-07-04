// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/library.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/serialization/escaped_text.hpp"

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;
using namespace Ttx::Lexical;

auto Library::lower(View::Bytes module, const Ttx::Type& root) -> Bool {
  View::Vector<Ttx::Type::Function> functions = root.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    if (!lower_function(module, root, functions[i])) {
      return False;
    }
  }
  return True;
}

auto Library::add_to(Tetrodotoxin::Linker::Linker& linker) const -> void {
  if (machine_code.get_size() != 0) {
    linker.add_section(Object::Section::Type::Program, machine_code);
  }

  if (string_data.get_size() != 0) {
    linker.add_section(Object::Section::Type::Strings, string_data);
  }

  for (Count i = 0; i < symbols.get_size(); i++) {
    linker.add_symbol(symbols[i]);
  }

  for (Count i = 0; i < relocations.get_size(); i++) {
    linker.add_relocation(relocations[i]);
  }
}

auto Library::append_header(Dynamic::Bytes& header) const -> void {
  for (Count i = 0; i < declarations.get_size(); i++) {
    const Declaration& declaration = declarations[i];
    View::Vector<Ttx::Type::Member> parameters =
        declaration.function.get_parameters();
    View::Vector<Ttx::Type::Member> result = declaration.function.get_result();
    View::Bytes return_type = "void"_view;
    if (!is_void_result(result) && result.get_size() == 1) {
      return_type = cpp_type(result[0].get_type());
    }

    header.concat("extern \"C\" "_view);
    header.concat(return_type);
    header.concat(" "_view);
    header.concat(declaration.symbol_name);
    header.concat("("_view);
    for (Count j = 0; j < parameters.get_size(); j++) {
      if (j != 0) {
        header.concat(", "_view);
      }
      header.concat(cpp_type(parameters[j].get_type()));
      header.concat(" "_view);
      header.concat(parameters[j].get_name());
    }
    header.concat(");\n"_view);
  }
}

auto Library::lower_function(
    View::Bytes module,
    const Ttx::Type& root,
    const Ttx::Type::Function& function) -> Bool {
  if (!function.has_body()) {
    return True;
  }

  if (!is_void_result(function.get_result())) {
    return set_error("Only void Library functions can lower to C++ today."_view);
  }

  View::Vector<Ttx::Type::Member> parameters = function.get_parameters();
  if (parameters.get_size() > 1) {
    return set_error(
        "Only one View[Bytes] function parameter can lower today."_view);
  }

  if (parameters.get_size() == 1 && !is_view_bytes(parameters[0].get_type())) {
    return set_error(
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
    if (!lower_block(root, function, blocks[i])) {
      return False;
    }
  }

  assembler.pop(Assembler::x86_64::Reg::RBP);
  assembler.pop(Assembler::x86_64::Reg::R12);
  assembler.pop(Assembler::x86_64::Reg::RBX);
  assembler.ret();

  Object::Symbol symbol = Object::Symbol::create_function(
      symbol_name(module, function.get_name()),
      Object::Symbol::Visibility::Global);
  symbol.set_range({function_start, machine_code.get_size() - function_start});
  symbols.insert(symbol);
  declarations.insert({symbol.get_name(), function});
  return True;
}

auto Library::lower_block(
    const Ttx::Type& root,
    const Ttx::Type::Function& function,
    Ttx::Type::Function::Block block) -> Bool {
  View::Vector<Token> tokens = block.get_tokens();
  for (Count i = 0; i < tokens.get_size();) {
    if (!lower_statement(root, function, tokens, i)) {
      return False;
    }
  }
  return True;
}

auto Library::lower_statement(
    const Ttx::Type& root,
    const Ttx::Type::Function& function,
    View::Vector<Token> tokens,
    Count& index) -> Bool {
  if (tokens[index].get_class() == Class::Type::Comment ||
      tokens[index].get_class() == Class::Type::Disabled) {
    index++;
    return True;
  }

  if (tokens[index].get_class() == Class::Type::Return) {
    index++;
    if (index >= tokens.get_size() ||
        tokens[index].get_class() != Class::Type::EndStatement) {
      return set_error("Expected `;` after return."_view);
    }
    index = tokens.get_size();
    return True;
  }

  return lower_foreign_call(root, function, tokens, index);
}

auto Library::lower_foreign_call(
    const Ttx::Type& root,
    const Ttx::Type::Function& function,
    View::Vector<Token> tokens,
    Count& index) -> Bool {
  if (tokens[index].get_class() != Class::Type::Type) {
    return set_error("Expected foreign type call target."_view);
  }

  const Ttx::Type* owner = root.find_type(tokens[index].get_text());
  if (owner == nullptr || !has_attribute(*owner, "isa"_view, "Foreign"_view)) {
    return set_error("Call target is not a foreign type."_view);
  }
  index++;

  if (index >= tokens.get_size() ||
      tokens[index].get_class() != Class::Type::CallOp) {
    return set_error("Expected `->` in foreign call."_view);
  }
  index++;

  if (index >= tokens.get_size() ||
      tokens[index].get_class() != Class::Type::Addressable) {
    return set_error("Expected foreign function name."_view);
  }
  const Token& function_name = tokens[index++];
  const Ttx::Type::Function* callee =
      owner->find_function(function_name.get_text());
  if (callee == nullptr) {
    return set_error("Foreign function could not be resolved."_view);
  }

  if (index >= tokens.get_size() ||
      tokens[index].get_class() != Class::Type::PackingStart) {
    return set_error("Expected `(` before foreign call arguments."_view);
  }
  index++;

  if (!lower_argument(function, tokens, index, *callee)) {
    return False;
  }

  if (index >= tokens.get_size() ||
      tokens[index].get_class() != Class::Type::PackingEnd) {
    return set_error("Expected `)` after foreign call arguments."_view);
  }
  index++;

  if (index >= tokens.get_size() ||
      tokens[index].get_class() != Class::Type::EndStatement) {
    return set_error("Expected `;` after foreign call."_view);
  }
  index++;

  call_external(function_name.get_text());
  return True;
}

auto Library::lower_argument(
    const Ttx::Type::Function& function,
    View::Vector<Token> tokens,
    Count& index,
    const Ttx::Type::Function& callee) -> Bool {
  View::Vector<Ttx::Type::Member> parameters = callee.get_parameters();
  if (parameters.get_size() != 1 || !is_view_bytes(parameters[0].get_type())) {
    return set_error(
        "Only foreign calls with one View[Bytes] argument can lower today."_view);
  }

  if (index >= tokens.get_size()) {
    return set_error("Expected foreign call argument."_view);
  }

  if (tokens[index].get_class() == Class::Type::String) {
    Bool valid = emit_string_argument(tokens[index].get_text());
    index++;
    return valid;
  }

  if (tokens[index].get_class() == Class::Type::Addressable) {
    Bool valid = emit_parameter_argument(function, tokens[index].get_text());
    index++;
    return valid;
  }

  return set_error("Unsupported foreign call argument."_view);
}

auto Library::emit_string_argument(View::Bytes token_text) -> Bool {
  if (token_text.get_size() < 2 || token_text[0] != '"' ||
      token_text[token_text.get_size() - 1] != '"') {
    return set_error("Malformed string literal."_view);
  }

  View::Bytes literal =
      token_text.slice(1, token_text.get_size() - 2);
  View::Bytes decoded = EscapedText::decode(arena, literal);
  const Count symbol = string_symbol(decoded);

  Assembler::x86_64 assembler(machine_code);
  assembler.read_only(Assembler::x86_64::Reg::RDI);
  relocations.insert(
      Object::Relocation::create_pc32(symbol, machine_code.get_size()));
  assembler.mov(Bits_64(decoded.get_size()), Assembler::x86_64::Reg::RSI);
  return True;
}

auto Library::emit_parameter_argument(
    const Ttx::Type::Function& function,
    View::Bytes name) -> Bool {
  View::Vector<Ttx::Type::Member> parameters = function.get_parameters();
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (parameters[i].get_name() != name) {
      continue;
    }

    if (!is_view_bytes(parameters[i].get_type())) {
      return set_error("Only View[Bytes] parameters can lower today."_view);
    }

    Assembler::x86_64 assembler(machine_code);
    assembler.mov(Assembler::x86_64::Reg::RBX, Assembler::x86_64::Reg::RDI);
    assembler.mov(Assembler::x86_64::Reg::R12, Assembler::x86_64::Reg::RSI);
    return True;
  }

  return set_error("Foreign call argument did not resolve to a parameter."_view);
}

auto Library::call_external(View::Bytes name) -> void {
  const Count symbol = external_symbol(name, Object::Symbol::Type::Function);
  Assembler::x86_64 assembler(machine_code);
  assembler.call();
  relocations.insert(
      Object::Relocation::create_plt32(symbol, machine_code.get_size()));
}

auto Library::string_symbol(View::Bytes value) -> Count {
  for (Count i = 0; i < symbols.get_size(); i++) {
    const Object::Symbol& symbol = symbols[i];
    if (symbol.get_location() != Object::Symbol::Location::Strings) {
      continue;
    }

    auto range = symbol.get_range();
    if (string_data.get_view().slice(range.start, range.size) == value) {
      return i;
    }
  }

  const Count offset = string_data.get_size();
  string_data.concat(value);
  const Count index = symbols.get_size();
  symbols.insert(Object::Symbol::create_string(
      local_symbol_name(value), {offset, value.get_size()}));
  return index;
}

auto Library::external_symbol(View::Bytes name, Object::Symbol::Type type)
    -> Count {
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols[i].get_location() == Object::Symbol::Location::External &&
        symbols[i].get_name() == name) {
      return i;
    }
  }

  const Count index = symbols.get_size();
  symbols.insert(Object::Symbol::create_external(keep(name), type));
  return index;
}

auto Library::symbol_name(View::Bytes module, View::Bytes function)
    -> View::Bytes {
  Managed::Bytes output(arena);
  output.concat("TTX_"_view);
  append_symbol_segment(output, module);
  output.append('_');
  append_symbol_segment(output, function);
  return output.get_view();
}

auto Library::local_symbol_name(View::Bytes value) -> View::Bytes {
  Managed::Bytes output(arena);
  output.concat(".Lttx_"_view);
  append_hex(output, Hash(value).get_value());
  output.append('_');
  append_hex(output, string_data.get_size());
  return output.get_view();
}

auto Library::append_symbol_segment(Managed::Bytes& output, View::Bytes value)
    -> void {
  for (Count i = 0; i < value.get_size(); i++) {
    Bits_8 c = value[i];
    const Bool alpha =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
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

auto Library::keep(View::Bytes bytes) -> View::Bytes {
  Bits_8* copy = arena.allocate(bytes.get_size());
  Data::copy(copy, bytes.get_data(), bytes.get_size());
  return View::Bytes(copy, bytes.get_size());
}

auto Library::set_error(View::Bytes message) -> Bool {
  if (error.get_size() == 0) {
    error.proxy(message);
  }
  return False;
}

auto Library::cpp_type(const Ttx::Type* type) -> View::Bytes {
  if (type == nullptr) {
    return "<unsupported>"_view;
  }

  const Ttx::Attribute* cpp = type->find_attribute("cpp"_view);
  return cpp == nullptr ? "<unsupported>"_view : cpp->get_value();
}

auto Library::has_attribute(
    const Ttx::Type& type,
    View::Bytes key,
    View::Bytes value) -> Bool {
  const Ttx::Attribute* attribute = type.find_attribute(key);
  if (attribute != nullptr && attribute->get_value() == value) {
    return True;
  }

  const Ttx::Type* canonical = type.canonical();
  if (canonical == nullptr || canonical == &type) {
    return False;
  }

  attribute = canonical->find_attribute(key);
  return attribute != nullptr && attribute->get_value() == value;
}

auto Library::is_view_bytes(const Ttx::Type* type) -> Bool {
  return type != nullptr && has_attribute(*type, "abi"_view, "view_bytes"_view);
}

auto Library::is_void_result(View::Vector<Ttx::Type::Member> result) -> Bool {
  if (result.is_empty()) {
    return True;
  }

  return result.get_size() == 1 && result[0].get_type() != nullptr &&
         has_attribute(*result[0].get_type(), "abi"_view, "void"_view);
}
