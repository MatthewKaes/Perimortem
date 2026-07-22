// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/target/spir_v/package.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/model/shader.hpp"
#include "tetrodotoxin/target/spir_v/compiler.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;

static auto append_word(Dynamic::Bytes& output, Unsigned_32 value) -> void {
  output.append(Unsigned_8(value & 0xFF));
  output.append(Unsigned_8((value >> 8) & 0xFF));
  output.append(Unsigned_8((value >> 16) & 0xFF));
  output.append(Unsigned_8((value >> 24) & 0xFF));
}

static auto metadata_bytes(const Target::SpirV::Metadata& metadata)
    -> Dynamic::Bytes {
  Dynamic::Bytes output("TTXI"_view);
  append_word(output, 1);
  append_word(output, Unsigned_32(metadata.get_execution()));
  append_word(output, Unsigned_32(metadata.get_input_count()));
  append_word(output, Unsigned_32(metadata.get_output_count()));
  append_word(output, Unsigned_32(metadata.get_push_count()));
  append_word(output, Unsigned_32(metadata.get_resource_count()));
  return output;
}

static auto metadata_path(View::Bytes module_path) -> Dynamic::Bytes {
  Dynamic::Bytes path(module_path);
  path.concat(".interface"_view);
  return path;
}

static auto seen(
    const Dynamic::Vector<const Abstract*>& visited,
    const Abstract& value) -> Bool {
  for (Count i = 0; i < visited.get_size(); i++) {
    if (visited[i] == &value) {
      return True;
    }
  }
  return False;
}

static auto compile_abstract(
    Allocator::Arena& arena,
    const Abstract& input,
    Dynamic::Vector<const Abstract*>& visited,
    Managed::Vector<Model::Terminal>& terminals,
    Managed::Vector<Model::Shaders::Product>& products) -> Bool {
  const Abstract& value = input.resolve();
  if (seen(visited, value)) {
    return True;
  }
  visited.insert(&value);

  if (value.is<Model::Namespace>()) {
    const auto& namespace_object = value.assume<Model::Namespace>();
    for (Count i = 0; i < namespace_object.get_export_count(); i++) {
      Bool compiled = compile_abstract(
          arena, namespace_object.get_export(i), visited, terminals, products);
      if (!compiled) {
        return False;
      }
    }
    return True;
  }
  if (!value.is<Model::Shader>()) {
    return True;
  }

  Dynamic::Vector<Target::SpirV::Module> modules;
  Bool compiled =
      Target::SpirV::Compiler::compile(value.assume<Model::Shader>(), modules);
  if (!compiled) {
    return False;
  }
  const auto& shader = value.assume<Model::Shader>();
  if (modules.get_size() != shader.get_stage_count()) {
    return False;
  }
  Managed::Vector<Model::Shaders::Product::Stage> stage_products(arena);
  stage_products.reset(modules.get_size());
  for (Count i = 0; i < modules.get_size(); i++) {
    Count module_terminal = terminals.get_size();
    terminals.insert(
        Model::Terminal(arena, modules[i].get_path(), modules[i].get_words()));
    Dynamic::Bytes interface_path = metadata_path(modules[i].get_path());
    Dynamic::Bytes interface_content =
        metadata_bytes(modules[i].get_metadata());
    if (!Model::Terminal::is_valid_path(interface_path)) {
      return False;
    }
    Count interface_terminal = terminals.get_size();
    terminals.insert(Model::Terminal(arena, interface_path, interface_content));
    const Abstract& stage = shader.get_stage(i);
    if (stage.is<Invalid>()) {
      return False;
    }
    stage_products.insert(
        Model::Shaders::Product::Stage(
            stage, module_terminal, interface_terminal));
  }
  products.insert(
      Model::Shaders::Product(arena, shader, stage_products.get_view()));
  return True;
}

auto Target::SpirV::PackageCompiler::compile(
    Allocator::Arena& arena,
    const Model::Namespace& exports,
    Managed::Vector<Model::Terminal>& terminals,
    Managed::Vector<Model::Shaders::Product>& products) -> Bool {
  Dynamic::Vector<const Abstract*> visited;
  return compile_abstract(arena, exports, visited, terminals, products);
}
