// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/window.hpp"

namespace Perimortem::Graphics::Vulkan {

class Swapchain;

// Owns the Vulkan instance, surface, physical device, logical device, graphics
// queue, and command pool for the lifetime of the application.
//
class Context {
 public:
  static auto create(const System::Window& window) -> Context;

  Context() = default;
  ~Context();
  Context(Context&&) noexcept;
  auto operator=(Context&&) noexcept -> Context&;
  Context(const Context&) = delete;
  auto operator=(const Context&) = delete;

  auto get_instance() const -> VkInstance;
  auto get_physical_device() const -> VkPhysicalDevice;
  auto get_device() const -> VkDevice;
  auto get_graphics_queue() const -> VkQueue;
  auto get_graphics_queue_family() const -> Bits_32;
  auto get_command_pool() const -> VkCommandPool;

  // Returns the index of the first memory type that satisfies both the type
  // filter bitmask and the required property flags. Returns UINT32_MAX if
  // no matching type is found.
  auto find_memory_type(
      Bits_32 type_filter,
      VkMemoryPropertyFlags properties) const -> Bits_32;

  // One-shot transfer commands are scoped by the caller: begin records a
  // transient command buffer, submit ends it and blocks until the GPU is done.
  auto begin_immediate_commands() const -> VkCommandBuffer;
  auto submit_immediate_commands(VkCommandBuffer command_buffer) const -> void;

 private:
  friend class Swapchain;

  auto get_surface() const -> VkSurfaceKHR;

  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphics_queue = VK_NULL_HANDLE;
  Bits_32 graphics_queue_family = 0;
  VkCommandPool command_pool = VK_NULL_HANDLE;
};

}  // namespace Perimortem::Graphics::Vulkan
