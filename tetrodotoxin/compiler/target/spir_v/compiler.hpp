// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/model/shader.hpp"
#include "tetrodotoxin/model/stages/required.hpp"

namespace Tetrodotoxin::Target::SpirV {

// Metadata is the target-independent evidence retained beside one emitted
// module. It summarizes the exact semantic interface consumed by lowering.
// the SPIR-V validator independently proves that the binary contains those
// facts.
class Metadata {
 public:
  constexpr Metadata() = default;
  constexpr Metadata(
      Model::Stages::Required::Execution execution,
      Count inputs,
      Count outputs,
      Count pushes,
      Count resources)
      : execution(execution),
        inputs(inputs),
        outputs(outputs),
        pushes(pushes),
        resources(resources) {}

  constexpr auto get_execution() const -> Model::Stages::Required::Execution {
    return execution;
  }
  constexpr auto get_input_count() const -> Count { return inputs; }
  constexpr auto get_output_count() const -> Count { return outputs; }
  constexpr auto get_push_count() const -> Count { return pushes; }
  constexpr auto get_resource_count() const -> Count { return resources; }

 private:
  Model::Stages::Required::Execution execution =
      Model::Stages::Required::Execution::Vertex;
  Count inputs = 0;
  Count outputs = 0;
  Count pushes = 0;
  Count resources = 0;
};

// Module owns one complete stage terminal in memory. Its logical path is
// normalized below the package root and remains stable across source-backed
// and archived Package construction.
class Module {
 public:
  Module(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Dynamic::Bytes words,
      Metadata metadata)
      : path(path),
        words(static_cast<Perimortem::Memory::Dynamic::Bytes&&>(words)),
        metadata(metadata) {}

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }
  constexpr auto get_words() const -> Perimortem::Core::View::Bytes {
    return words;
  }
  constexpr auto get_metadata() const -> const Metadata& { return metadata; }

 private:
  Perimortem::Memory::Dynamic::Bytes path;
  Perimortem::Memory::Dynamic::Bytes words;
  Metadata metadata;
};

// Compiler derives one physical SPIR-V Representation per Stage directly from
// the Shader, Render, Callable, Addressable, Type, and common Body contracts.
// It retains no semantic mirror and rejects a whole Shader transaction if any
// stage lacks an explicit supported representation.
class Compiler {
 public:
  static auto compile(
      const Model::Shader& shader,
      Perimortem::Memory::Dynamic::Vector<Module>& modules) -> Bool;
};

}  // namespace Tetrodotoxin::Target::SpirV
