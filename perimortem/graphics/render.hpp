// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Describes the backend-independent inputs needed to construct a graphics
// pipeline. Stages, host ranges, descriptor locations, and reflected fields
// are Graphics facts even though one backend may translate them into Vulkan
// objects and another may use a different native representation.
//
// The current Program is a borrowed descriptor intended for static C++ data.
// Every referenced module, name, and table must outlive it. That is sufficient
// for bringing up the C++ rendering path, but it is not the eventual live-edit
// artifact we need to cover all of our complex scenarios. Editor compilation
// needs an owning immutable candidate with stable program identity and some
// generation metadata so a backend can build it, inspect it, and install it at
// a frame boundary without invalidating the active program. That ownership
// belongs in Graphics directly rather than being exposed to Vulkan or TTX.
class Render {
 public:
  // Shader stage used by a module or host-input range.
  enum class Stage : Bits_32 {
    Vertex,
    Pixel,
  };

  // One SPIR-V module and the entry point selected from it.
  class Module {
   public:
    constexpr Module() = default;
    constexpr Module(
        Stage stage,
        const Bits_32* SpirV,
        const Count* SpirV_size,
        const char* entry)
        : stage(stage), SpirV(SpirV), SpirV_size(SpirV_size), entry(entry) {}

    constexpr auto get_stage() const -> Stage { return stage; }
    constexpr auto get_SpirV() const -> const Bits_32* { return SpirV; }
    constexpr auto get_SpirV_size() const -> const Count* { return SpirV_size; }
    constexpr auto get_entry() const -> const char* { return entry; }

   private:
    Stage stage = Stage::Vertex;
    const Bits_32* SpirV = nullptr;
    const Count* SpirV_size = nullptr;
    const char* entry = nullptr;
  };

  // Byte range made visible to one or more shader stages as push constants.
  class HostInputRange {
   public:
    constexpr HostInputRange() = default;
    constexpr HostInputRange(
        Count offset,
        Count size,
        Core::View::Vector<Stage> stages)
        : offset(offset), size(size), stages(stages) {}

    constexpr auto get_offset() const -> Count { return offset; }
    constexpr auto get_size() const -> Count { return size; }
    constexpr auto get_stages() const -> Core::View::Vector<Stage> {
      return stages;
    }

   private:
    Count offset = 0;
    Count size = 0;
    Core::View::Vector<Stage> stages;
  };

  // Named descriptor location expected by the generated render program.
  class DescriptorBinding {
   public:
    constexpr DescriptorBinding() = default;
    constexpr DescriptorBinding(const char* name, Count set, Count slot)
        : name(name), set(set), slot(slot) {}

    constexpr auto get_name() const -> const char* { return name; }
    constexpr auto get_set() const -> Count { return set; }
    constexpr auto get_slot() const -> Count { return slot; }

   private:
    const char* name = nullptr;
    Count set = 0;
    Count slot = 0;
  };

  // Named field within the host-input byte layout.
  class HostField {
   public:
    constexpr HostField() = default;
    constexpr HostField(const char* name, Count offset, Count size)
        : name(name), offset(offset), size(size) {}

    constexpr auto get_name() const -> const char* { return name; }
    constexpr auto get_offset() const -> Count { return offset; }
    constexpr auto get_size() const -> Count { return size; }

   private:
    const char* name = nullptr;
    Count offset = 0;
    Count size = 0;
  };

  // A non-owning view of one complete pipeline description. Program does not
  // cache a backend object and cannot indicate that installation succeeded.
  // Its only lifetime contract is that every referenced array and string
  // remains alive while a backend reads the descriptor.
  class Program {
   public:
    constexpr Program() = default;
    constexpr Program(
        Core::View::Vector<Module> modules,
        Core::View::Vector<HostInputRange> host_input_ranges,
        Core::View::Vector<DescriptorBinding> descriptors,
        Core::View::Vector<HostField> host_fields,
        Count vertex_count)
        : modules(modules),
          host_input_ranges(host_input_ranges),
          descriptors(descriptors),
          host_fields(host_fields),
          vertex_count(vertex_count) {}

    constexpr auto get_modules() const -> Core::View::Vector<Module> {
      return modules;
    }

    constexpr auto get_host_input_ranges() const
        -> Core::View::Vector<HostInputRange> {
      return host_input_ranges;
    }

    constexpr auto get_descriptors() const
        -> Core::View::Vector<DescriptorBinding> {
      return descriptors;
    }

    constexpr auto get_host_fields() const -> Core::View::Vector<HostField> {
      return host_fields;
    }

    constexpr auto get_vertex_count() const -> Count { return vertex_count; }

   private:
    Core::View::Vector<Module> modules;
    Core::View::Vector<HostInputRange> host_input_ranges;
    Core::View::Vector<DescriptorBinding> descriptors;
    Core::View::Vector<HostField> host_fields;
    Count vertex_count = 0;
  };

  constexpr Render() = default;
  constexpr Render(const Program& program) : program(&program) {}

  constexpr auto get_program() const -> const Program* { return program; }

 private:
  const Program* program = nullptr;
};

}  // namespace Perimortem::Graphics
