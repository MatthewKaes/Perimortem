// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;

auto Shader::lower(View::Bytes module, const Ttx::Type& root) -> Bool {
  if (!has_attribute(root, "isa"_view, "Shader"_view)) {
    return True;
  }

  View::Vector<Ttx::Type::Function> stages = root.get_functions();
  if (stages.is_empty()) {
    return set_error("Shader source did not declare any stages."_view);
  }

  for (Count i = 0; i < stages.get_size(); i++) {
    if (!lower_stage(module, root, stages[i])) {
      return False;
    }
  }

  return True;
}

auto Shader::add_to(Tetrodotoxin::Linker::Linker& linker) const -> void {
  if (read_only.get_size() != 0) {
    linker.add_section(Object::Section::Type::ReadOnly, read_only);
  }

  for (Count i = 0; i < symbols.get_size(); i++) {
    linker.add_symbol(symbols[i]);
  }
}

auto Shader::lower_stage(
    View::Bytes module,
    const Ttx::Type& shader,
    const Ttx::Type::Function& stage) -> Bool {
  Assembler::SpirV::ExecutionModel model;
  if (stage.get_name() == "vertex"_view) {
    model = Assembler::SpirV::ExecutionModel::Vertex;
  } else if (stage.get_name() == "pixel"_view) {
    model = Assembler::SpirV::ExecutionModel::Fragment;
  } else {
    return set_error("Only vertex and pixel shader stages can lower today."_view);
  }

  Dynamic::Bytes words;
  Assembler::SpirV assembler(words);
  assembler.begin_module(5);
  assembler.capability(Assembler::SpirV::Capability::Shader);
  assembler.memory_model(
      Assembler::SpirV::AddressingModel::Logical,
      Assembler::SpirV::MemoryModel::GLSL450);
  assembler.entry_point(model, 1, "main"_view);
  if (model == Assembler::SpirV::ExecutionModel::Fragment) {
    assembler.execution_mode(1, Assembler::SpirV::ExecutionMode::OriginUpperLeft);
  }
  assembler.type_void(2);
  assembler.type_function(3, 2);
  assembler.function(2, 1, Assembler::SpirV::FunctionControl::None, 3);
  assembler.label(4);
  assembler.return_void();
  assembler.function_end();

  if (!Assembler::SpirV::is_valid_module(words)) {
    return set_error("Shader compiler emitted invalid SPIR-V."_view);
  }

  const Count offset = read_only.get_size();
  read_only.concat(words.get_view());
  symbols.insert(Object::Symbol::create_read_only(
      symbol_name(module, shader.get_name(), stage.get_name()),
      {offset, words.get_size()}));
  return True;
}

auto Shader::symbol_name(
    View::Bytes module,
    View::Bytes shader,
    View::Bytes stage) -> View::Bytes {
  Managed::Bytes output(arena);
  output.concat("TTX_shader_"_view);
  append_symbol_segment(output, module);
  output.append('_');
  append_symbol_segment(output, shader);
  output.append('_');
  append_symbol_segment(output, stage);
  output.concat("_spirv"_view);
  return output.get_view();
}

auto Shader::append_symbol_segment(Managed::Bytes& output, View::Bytes value)
    -> void {
  for (Count i = 0; i < value.get_size(); i++) {
    Bits_8 c = value[i];
    const Bool alpha =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const Bool digit = c >= '0' && c <= '9';
    output.append(alpha || digit || c == '_' ? c : Bits_8('_'));
  }
}

auto Shader::set_error(View::Bytes message) -> Bool {
  if (error.get_size() == 0) {
    error.proxy(message);
  }
  return False;
}

auto Shader::has_attribute(
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
