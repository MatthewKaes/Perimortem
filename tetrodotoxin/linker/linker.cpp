// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "tetrodotoxin/compiler/library.hpp"
#include "tetrodotoxin/compiler/shader.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;

static auto is_void_result(View::Vector<Ttx::Type::Member> result) -> Bool {
  if (result.is_empty()) {
    return True;
  }

  const Ttx::Type* type = result[0].get_type();
  return result.get_size() == 1 && type != nullptr &&
         type->attribute_equals("abi"_view, "void"_view);
}

static auto cpp_type(const Ttx::Type* type) -> View::Bytes {
  if (type == nullptr) {
    return "<unsupported>"_view;
  }

  const Ttx::Attribute* cpp = type->resolve_attribute("cpp"_view);
  return cpp == nullptr ? "<unsupported>"_view : cpp->get_value();
}

auto Linker::add(const Library& library) -> void {
  Bits_16 program_section = 0;
  if (library.get_machine_code().get_size() != 0) {
    program_section =
        add_section(Object::Section::Type::Program, library.get_machine_code());
  }

  Bits_16 string_section = 0;
  if (library.get_string_data().get_size() != 0) {
    string_section =
        add_section(Object::Section::Type::Strings, library.get_string_data());
  }

  View::Vector<Symbol::Function> functions = library.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    Object::Symbol symbol = Object::Symbol::create_function(
        functions[i].name, program_section,
        Object::Symbol::Visibility::Global);
    symbol.set_range(functions[i].range);
    add_symbol(symbol);
  }

  const Count string_symbol_offset = symbol_count;
  View::Vector<Symbol::String> strings = library.get_strings();
  for (Count i = 0; i < strings.get_size(); i++) {
    add_symbol(Object::Symbol::create_string(
        strings[i].name, string_section, strings[i].range));
  }

  const Count external_symbol_offset = symbol_count;
  View::Vector<Symbol::External> externals = library.get_externals();
  for (Count i = 0; i < externals.get_size(); i++) {
    add_symbol(Object::Symbol::create_external(
        externals[i].name, Object::Symbol::Type::Function));
  }

  View::Vector<Symbol::Relocation> relocations = library.get_relocations();
  for (Count i = 0; i < relocations.get_size(); i++) {
    const Symbol::Relocation& relocation = relocations[i];
    Count symbol_index = 0;
    switch (relocation.target) {
    case Symbol::Relocation::Target::String:
      symbol_index = string_symbol_offset + relocation.target_index;
      break;
    case Symbol::Relocation::Target::External:
      symbol_index = external_symbol_offset + relocation.target_index;
      break;
    }

    switch (relocation.type) {
    case Symbol::Relocation::Type::Pc32:
      add_relocation(Object::Relocation::create_pc32(
          program_section, symbol_index, relocation.code_offset));
      break;
    case Symbol::Relocation::Type::Plt32:
      add_relocation(Object::Relocation::create_plt32(
          program_section, symbol_index, relocation.code_offset));
      break;
    }
  }
}

auto Linker::add(const Shader& shader) -> void {
  Bits_16 read_only_section = 0;
  if (shader.get_read_only().get_size() != 0) {
    read_only_section =
        add_section(Object::Section::Type::ReadOnly, shader.get_read_only());
  }

  View::Vector<Symbol::Stage> stages = shader.get_stages();
  for (Count i = 0; i < stages.get_size(); i++) {
    add_symbol(Object::Symbol::create_read_only(
        stages[i].name, read_only_section, stages[i].range));
  }
}

auto Linker::add_section(Object::Section::Type type, View::Bytes data)
    -> Bits_16 {
  return format.add_section({type, data});
}

auto Linker::add_section(Object::Section section) -> Bits_16 {
  return format.add_section(section);
}

auto Linker::add_symbol(Object::Symbol symbol) -> void {
  format.add_symbol(symbol);
  symbol_count++;
}

auto Linker::add_relocation(Object::Relocation relocation) -> void {
  format.add_relocation(relocation);
}

auto Linker::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  return format.build_library(object_name);
}

auto Linker::append_header(
    Dynamic::Bytes& header,
    const Library& library) const -> void {
  View::Vector<Symbol::Function> functions = library.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    const Symbol::Function& function = functions[i];
    View::Vector<Ttx::Type::Member> parameters =
        function.source.get_parameters();
    View::Vector<Ttx::Type::Member> result = function.source.get_result();
    View::Bytes return_type = "void"_view;
    if (!is_void_result(result) && result.get_size() == 1) {
      return_type = cpp_type(result[0].get_type());
    }

    header.concat("extern \"C\" "_view);
    header.concat(return_type);
    header.concat(" "_view);
    header.concat(function.name);
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

auto Linker::reset() -> void {
  format.reset();
  symbol_count = 0;
}
