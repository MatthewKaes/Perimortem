// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/swapchain.hpp"

using namespace Perimortem::Graphics::Vulkan;

namespace {

auto choose_surface_format(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
  Bits_32 count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device, surface, &count, nullptr);

  VkSurfaceFormatKHR formats[32] = {};
  Bits_32 clamped = count < 32 ? count : 32;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device, surface, &clamped, formats);

  for (Bits_32 i = 0; i < clamped; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return formats[i];
    }
  }
  return formats[0];
}

auto choose_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
    -> VkPresentModeKHR {
  Bits_32 count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &count, nullptr);

  VkPresentModeKHR modes[8] = {};
  Bits_32 clamped = count < 8 ? count : 8;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &clamped, modes);

  for (Bits_32 i = 0; i < clamped; i++) {
    if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

}  // namespace

auto Swapchain::build(
    const Context& context,
    Bits_32 width,
    Bits_32 height,
    VkSwapchainKHR old_swapchain) -> Swapchain {
  Swapchain sc;
  sc.device = context.get_device();

  const VkSurfaceFormatKHR surface_format = choose_surface_format(
      context.get_physical_device(), context.get_surface());
  const VkPresentModeKHR present_mode =
      choose_present_mode(context.get_physical_device(), context.get_surface());

  sc.format = surface_format.format;
  sc.extent = {width, height};

  VkSurfaceCapabilitiesKHR caps = {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      context.get_physical_device(), context.get_surface(), &caps);

  // Request one more than the minimum to avoid stalling on the driver.
  Bits_32 image_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
    image_count = caps.maxImageCount;
  }

  const Bits_32 queue_family = context.get_graphics_queue_family();

  VkSwapchainCreateInfoKHR info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  info.surface = context.get_surface();
  info.minImageCount = image_count;
  info.imageFormat = sc.format;
  info.imageColorSpace = surface_format.colorSpace;
  info.imageExtent = sc.extent;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 1;
  info.pQueueFamilyIndices = &queue_family;
  info.preTransform = caps.currentTransform;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode = present_mode;
  info.clipped = VK_TRUE;
  info.oldSwapchain = old_swapchain;

  vkCreateSwapchainKHR(sc.device, &info, nullptr, &sc.swapchain);

  sc.image_count = 0;
  vkGetSwapchainImagesKHR(sc.device, sc.swapchain, &sc.image_count, nullptr);
  if (sc.image_count > max_images) {
    sc.image_count = max_images;
  }
  vkGetSwapchainImagesKHR(sc.device, sc.swapchain, &sc.image_count, sc.images);

  for (Bits_32 i = 0; i < sc.image_count; i++) {
    VkImageViewCreateInfo view_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = sc.images[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = sc.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    vkCreateImageView(sc.device, &view_info, nullptr, &sc.image_views[i]);
  }

  return sc;
}

auto Swapchain::create(const Context& context, Bits_32 width, Bits_32 height)
    -> Swapchain {
  return build(context, width, height, VK_NULL_HANDLE);
}

auto Swapchain::recreate(const Context& context, Bits_32 width, Bits_32 height)
    -> void {
  const VkSwapchainKHR old = swapchain;

  // Destroy image views but leave the old swapchain alive for oldSwapchain.
  for (Bits_32 i = 0; i < image_count; i++) {
    vkDestroyImageView(device, image_views[i], nullptr);
    image_views[i] = VK_NULL_HANDLE;
  }
  image_count = 0;
  swapchain = VK_NULL_HANDLE;

  *this = build(context, width, height, old);

  vkDestroySwapchainKHR(device, old, nullptr);
}

auto Swapchain::destroy(VkDevice dev) -> void {
  for (Bits_32 i = 0; i < image_count; i++) {
    vkDestroyImageView(dev, image_views[i], nullptr);
  }
  vkDestroySwapchainKHR(dev, swapchain, nullptr);
}

Swapchain::~Swapchain() {
  if (device && swapchain) {
    destroy(device);
  }
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : device(other.device),
      swapchain(other.swapchain),
      format(other.format),
      extent(other.extent),
      image_count(other.image_count) {
  for (Bits_32 i = 0; i < image_count; i++) {
    images[i] = other.images[i];
    image_views[i] = other.image_views[i];
  }
  other.swapchain = VK_NULL_HANDLE;
}

auto Swapchain::operator=(Swapchain&& other) noexcept -> Swapchain& {
  if (this != &other) {
    if (device && swapchain) {
      destroy(device);
    }
    device = other.device;
    swapchain = other.swapchain;
    format = other.format;
    extent = other.extent;
    image_count = other.image_count;
    for (Bits_32 i = 0; i < image_count; i++) {
      images[i] = other.images[i];
      image_views[i] = other.image_views[i];
    }
    other.swapchain = VK_NULL_HANDLE;
  }
  return *this;
}

auto Swapchain::acquire_next_image(VkSemaphore signal_semaphore) const
    -> Bits_32 {
  Bits_32 index = 0;
  const VkResult result = vkAcquireNextImageKHR(
      device, swapchain, UINT64_MAX, signal_semaphore, VK_NULL_HANDLE, &index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return UINT32_MAX;
  }
  return index;
}

auto Swapchain::get_swapchain() const -> VkSwapchainKHR {
  return swapchain;
}
auto Swapchain::get_extent() const -> VkExtent2D {
  return extent;
}
auto Swapchain::get_format() const -> VkFormat {
  return format;
}
auto Swapchain::get_image_view(Bits_32 index) const -> VkImageView {
  return image_views[index];
}
auto Swapchain::get_image_count() const -> Bits_32 {
  return image_count;
}
