// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/graphics/render.hpp"

namespace Perimortem::Graphics::Vulkan {

class ShaderProgram {
 public:
  static auto create(
      VkDevice device,
      VkFormat color_format,
      const Graphics::Render::Program& source,
      Core::View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
      -> ShaderProgram;

  ShaderProgram() = default;
  ~ShaderProgram();
  ShaderProgram(ShaderProgram&&) noexcept;
  auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;
  ShaderProgram(const ShaderProgram&) = delete;
  auto operator=(const ShaderProgram&) = delete;

  auto bind(VkCommandBuffer command_buffer) const -> void;
  auto bind_descriptor_set(
      VkCommandBuffer command_buffer,
      VkDescriptorSet descriptor_set,
      Count descriptor_index = 0) const -> void;
  auto push_constants(
      VkCommandBuffer command_buffer,
      const void* source,
      Count size,
      Count range_index = 0) const -> void;
  auto draw(VkCommandBuffer command_buffer) const -> void;

  auto get_layout() const -> VkPipelineLayout;
  auto get_pipeline() const -> VkPipeline;

 private:
  class PushConstantRange {
   public:
    constexpr PushConstantRange() = default;
    constexpr PushConstantRange(
        VkShaderStageFlags stage_flags,
        Count offset,
        Count size)
        : stage_flags(stage_flags), offset(offset), size(size) {}

    constexpr auto get_stage_flags() const -> VkShaderStageFlags {
      return stage_flags;
    }
    constexpr auto get_offset() const -> Count { return offset; }
    constexpr auto get_size() const -> Count { return size; }

   private:
    VkShaderStageFlags stage_flags = 0;
    Count offset = 0;
    Count size = 0;
  };

  class DescriptorBinding {
   public:
    constexpr DescriptorBinding() = default;
    constexpr DescriptorBinding(Count set, Count slot) : set(set), slot(slot) {}

    constexpr auto get_set() const -> Count { return set; }
    constexpr auto get_slot() const -> Count { return slot; }

   private:
    Count set = 0;
    Count slot = 0;
  };

  static constexpr Count max_push_constant_ranges = 8;
  static constexpr Count max_descriptor_bindings = 8;

  VkDevice device = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  Core::Static::Vector<PushConstantRange, max_push_constant_ranges>
      push_constant_ranges;
  Core::Static::Vector<DescriptorBinding, max_descriptor_bindings> descriptors;
  Count push_constant_count = 0;
  Count descriptor_count = 0;
  Count vertex_count = 0;
};

}  // namespace Perimortem::Graphics::Vulkan
