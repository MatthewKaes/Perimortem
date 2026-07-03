// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/vulkan/context.hpp"

namespace Perimortem::Graphics::Vulkan {

// Uploads a Graphics::Image to a device-local VkImage and owns the associated
// view, sampler, descriptor set layout, descriptor pool, and descriptor set.
// Intended for a single combined-image-sampler binding at set 0, binding 0.
class Texture {
 public:
  static auto create(const Context& ctx, const Graphics::Image& image)
      -> Texture;

  Texture() = default;
  ~Texture();
  Texture(Texture&&) noexcept;
  auto operator=(Texture&&) noexcept -> Texture&;
  Texture(const Texture&) = delete;
  auto operator=(const Texture&) = delete;

  auto get_descriptor_set() const -> VkDescriptorSet;
  auto get_descriptor_set_layout() const -> VkDescriptorSetLayout;

 private:
  VkDevice device = VK_NULL_HANDLE;

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

}  // namespace Perimortem::Graphics::Vulkan
