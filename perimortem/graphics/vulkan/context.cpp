// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/context.hpp"

#include <vulkan/vulkan_wayland.h>

using namespace Perimortem::Graphics::Vulkan;

namespace {

constexpr const char* instance_extensions[] = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
};

constexpr const char* device_extensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef PERI_DEBUG
constexpr const char* validation_layers[] = {
  "VK_LAYER_KHRONOS_validation",
};
constexpr Bits_32 validation_layer_count = 1;
#else
constexpr const char** validation_layers = nullptr;
constexpr Bits_32 validation_layer_count = 0;
#endif

auto select_physical_device(VkInstance instance, VkSurfaceKHR surface)
    -> VkPhysicalDevice {
  Bits_32 count = 0;
  vkEnumeratePhysicalDevices(instance, &count, nullptr);

  VkPhysicalDevice devices[16] = {};
  Bits_32 clamped = count < 16 ? count : 16;
  vkEnumeratePhysicalDevices(instance, &clamped, devices);

  // Prefer discrete GPUs; fall back to any device that can present.
  VkPhysicalDevice fallback = VK_NULL_HANDLE;
  for (Bits_32 i = 0; i < clamped; i++) {
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(devices[i], &props);

    // Check that a queue family supports both graphics and present.
    Bits_32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        devices[i], &family_count, nullptr);

    VkQueueFamilyProperties families[32] = {};
    Bits_32 fc = family_count < 32 ? family_count : 32;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &fc, families);

    Bool usable = False;
    for (Bits_32 f = 0; f < fc; f++) {
      if (!(families[f].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
        continue;
      }
      VkBool32 present_supported = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(
          devices[i], f, surface, &present_supported);
      if (present_supported) {
        usable = True;
        break;
      }
    }

    if (!usable) {
      continue;
    }
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return devices[i];
    }
    fallback = devices[i];
  }
  return fallback;
}

auto find_graphics_queue_family(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> Bits_32 {
  Bits_32 count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

  VkQueueFamilyProperties families[32] = {};
  Bits_32 clamped = count < 32 ? count : 32;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &clamped, families);

  for (Bits_32 i = 0; i < clamped; i++) {
    if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }
    VkBool32 present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present);
    if (present) {
      return i;
    }
  }
  return UINT32_MAX;
}

}  // namespace

auto Context::create(void* wl_display, void* wl_surface) -> Context {
  Context ctx;

  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "Perimortem";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = 2;
  instance_info.ppEnabledExtensionNames = instance_extensions;
  instance_info.enabledLayerCount = validation_layer_count;
  instance_info.ppEnabledLayerNames = validation_layers;

  vkCreateInstance(&instance_info, nullptr, &ctx.instance);

  VkWaylandSurfaceCreateInfoKHR surface_info = {
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
  surface_info.display = static_cast<struct wl_display*>(wl_display);
  surface_info.surface = static_cast<struct wl_surface*>(wl_surface);
  vkCreateWaylandSurfaceKHR(ctx.instance, &surface_info, nullptr, &ctx.surface);

  ctx.physical_device = select_physical_device(ctx.instance, ctx.surface);
  ctx.graphics_queue_family =
      find_graphics_queue_family(ctx.physical_device, ctx.surface);

  constexpr Real_32 queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueFamilyIndex = ctx.graphics_queue_family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  dynamic_rendering.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features sync2 = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  sync2.synchronization2 = VK_TRUE;
  sync2.pNext = &dynamic_rendering;

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &sync2;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount = 1;
  device_info.ppEnabledExtensionNames = device_extensions;

  vkCreateDevice(ctx.physical_device, &device_info, nullptr, &ctx.device);
  vkGetDeviceQueue(
      ctx.device, ctx.graphics_queue_family, 0, &ctx.graphics_queue);

  VkCommandPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = ctx.graphics_queue_family;
  vkCreateCommandPool(ctx.device, &pool_info, nullptr, &ctx.command_pool);

  return ctx;
}

Context::~Context() {
  if (!device) {
    return;
  }
  vkDestroyCommandPool(device, command_pool, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

Context::Context(Context&& other) noexcept
    : instance(other.instance),
      surface(other.surface),
      physical_device(other.physical_device),
      device(other.device),
      graphics_queue(other.graphics_queue),
      graphics_queue_family(other.graphics_queue_family),
      command_pool(other.command_pool) {
  other.device = VK_NULL_HANDLE;
}

auto Context::operator=(Context&& other) noexcept -> Context& {
  if (this != &other) {
    this->~Context();
    instance = other.instance;
    surface = other.surface;
    physical_device = other.physical_device;
    device = other.device;
    graphics_queue = other.graphics_queue;
    graphics_queue_family = other.graphics_queue_family;
    command_pool = other.command_pool;
    other.device = VK_NULL_HANDLE;
  }
  return *this;
}

auto Context::get_instance() const -> VkInstance {
  return instance;
}
auto Context::get_physical_device() const -> VkPhysicalDevice {
  return physical_device;
}
auto Context::get_device() const -> VkDevice {
  return device;
}
auto Context::get_surface() const -> VkSurfaceKHR {
  return surface;
}
auto Context::get_graphics_queue() const -> VkQueue {
  return graphics_queue;
}
auto Context::get_graphics_queue_family() const -> Bits_32 {
  return graphics_queue_family;
}
auto Context::get_command_pool() const -> VkCommandPool {
  return command_pool;
}

auto Context::find_memory_type(Bits_32 type_filter, VkMemoryPropertyFlags props)
    const -> Bits_32 {
  VkPhysicalDeviceMemoryProperties mem_props = {};
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
  for (Bits_32 i = 0; i < mem_props.memoryTypeCount; i++) {
    const Bool type_matches = Bool((type_filter & (1u << i)) != 0);
    const Bool props_match =
        Bool((mem_props.memoryTypes[i].propertyFlags & props) == props);
    if (type_matches & props_match) {
      return i;
    }
  }
  return UINT32_MAX;
}
