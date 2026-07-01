// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Perimortem::Graphics {

// Runtime render metadata shared by generated TTX headers and the renderer.
// Tetrodotoxin may generate these objects, but Graphics owns the runtime shape.
class Render {
 public:
  enum class Stage : Bits_32 {
    Vertex,
    Pixel,
  };

  class Module {
   public:
    constexpr Module() = default;
    constexpr Module(
        Stage stage,
        const Bits_32* spirv,
        const Count* spirv_size,
        const char* entry)
        : stage(stage),
          spirv(spirv),
          spirv_size(spirv_size),
          entry(entry) {}

    constexpr auto get_stage() const -> Stage { return stage; }
    constexpr auto get_spirv() const -> const Bits_32* { return spirv; }
    constexpr auto get_spirv_size() const -> const Count* {
      return spirv_size;
    }
    constexpr auto get_entry() const -> const char* { return entry; }

   private:
    Stage stage = Stage::Vertex;
    const Bits_32* spirv = nullptr;
    const Count* spirv_size = nullptr;
    const char* entry = nullptr;
  };

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
    constexpr auto is_valid() const -> Bool { return !modules.is_empty(); }

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
  constexpr auto is_valid() const -> Bool {
    return program && program->is_valid();
  }

 private:
  const Program* program = nullptr;
};

}  // namespace Perimortem::Graphics
