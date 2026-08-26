// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "perimortem/graphics/frame/resource.hpp"
#include "perimortem/graphics/size_2d.hpp"
#include "perimortem/vulkan/context.hpp"

namespace Perimortem::Vulkan {

// Uploads one retained pixel resource to a device local VkImage and owns the
// associated view, sampler, descriptor pool, and descriptor set. The calling
// pipeline supplies its image extent and shared layout so every cached resource
// remains compatible with the same program contract.
class Texture {
 public:
  static auto create(
      const Context& ctx,
      const Graphics::Frame::Resource& resource,
      VkDescriptorSetLayout descriptor_set_layout) -> Texture;

  Texture() = default;
  ~Texture();
  Texture(Texture&&) noexcept;
  auto operator=(Texture&&) noexcept -> Texture&;
  Texture(const Texture&) = delete;
  auto operator=(const Texture&) = delete;

  auto get_descriptor_set() const -> VkDescriptorSet;

 private:
  VkDevice device = VK_NULL_HANDLE;

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

}  // namespace Perimortem::Vulkan
