// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"

#include "perimortem/graphics/ttx_shader.hpp"

namespace Perimortem::Graphics::Vulkan {

class ShaderProgram {
 public:
  static auto create(
      VkDevice device,
      VkFormat color_format,
      const Ttx::ShaderProgramInfo& source,
      Core::View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
      -> ShaderProgram;

  ShaderProgram() = default;
  ~ShaderProgram();
  ShaderProgram(ShaderProgram&&) noexcept;
  auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;
  ShaderProgram(const ShaderProgram&) = delete;
  auto operator=(const ShaderProgram&) = delete;

  auto bind(VkCommandBuffer cmd) const -> void;
  auto bind_descriptor_set(
      VkCommandBuffer cmd,
      VkDescriptorSet descriptor_set,
      Count descriptor_index = 0) const -> void;
  auto push_constants(
      VkCommandBuffer cmd,
      const void* source,
      Count size,
      Count range_index = 0) const -> void;
  auto draw(VkCommandBuffer cmd) const -> void;

  auto get_layout() const -> VkPipelineLayout;
  auto get_pipeline() const -> VkPipeline;

 private:
  VkDevice device = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  const Ttx::ShaderProgramInfo* source = nullptr;
};

}  // namespace Perimortem::Graphics::Vulkan
