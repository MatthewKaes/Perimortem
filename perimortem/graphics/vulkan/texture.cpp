// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/texture.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Graphics::Vulkan;

static auto require_success(
    VkResult result,
    Perimortem::Core::View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Perimortem::Core::Diagnostics::Log::fatal(message);
  }
}

static auto allocate_memory(
    const Context& ctx,
    VkMemoryRequirements requirements,
    VkMemoryPropertyFlags properties) -> VkDeviceMemory {
  VkMemoryAllocateInfo info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  info.allocationSize = requirements.size;
  info.memoryTypeIndex =
      ctx.find_memory_type(requirements.memoryTypeBits, properties);
  if (info.memoryTypeIndex == UINT32_MAX) {
    Perimortem::Core::Diagnostics::Log::fatal(
        "Vulkan: No compatible texture memory type found."_view);
  }
  VkDeviceMemory memory = VK_NULL_HANDLE;
  require_success(
      vkAllocateMemory(ctx.get_device(), &info, nullptr, &memory),
      "Vulkan: Failed to allocate texture memory."_view);
  return memory;
}

static auto transition_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags2 source_stage,
    VkAccessFlags2 source_access,
    VkPipelineStageFlags2 destination_stage,
    VkAccessFlags2 destination_access) -> void {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = source_stage;
  barrier.srcAccessMask = source_access;
  barrier.dstStageMask = destination_stage;
  barrier.dstAccessMask = destination_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dependency.imageMemoryBarrierCount = 1;
  dependency.pImageMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(command_buffer, &dependency);
}

auto Texture::create(const Context& ctx, const Graphics::Image& source)
    -> Texture {
  Texture texture;
  texture.device = ctx.get_device();

  const Bits_32 width = source.get_width();
  const Bits_32 height = source.get_height();
  const VkDeviceSize image_size = VkDeviceSize(width) * height * 4;

  // Staging buffer: host-visible, host-coherent.
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = image_size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer staging = VK_NULL_HANDLE;
  require_success(
      vkCreateBuffer(ctx.get_device(), &buffer_info, nullptr, &staging),
      "Vulkan: Failed to create texture staging buffer."_view);

  VkMemoryRequirements staging_requirements = {};
  vkGetBufferMemoryRequirements(
      ctx.get_device(), staging, &staging_requirements);

  constexpr VkMemoryPropertyFlags host_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkDeviceMemory staging_memory =
      allocate_memory(ctx, staging_requirements, host_flags);
  require_success(
      vkBindBufferMemory(ctx.get_device(), staging, staging_memory, 0),
      "Vulkan: Failed to bind texture staging memory."_view);

  void* mapped = nullptr;
  require_success(
      vkMapMemory(ctx.get_device(), staging_memory, 0, image_size, 0, &mapped),
      "Vulkan: Failed to map texture staging memory."_view);
  const auto pixels = source.get_pixels();
  // Pixel is 4 bytes RGBA — layout matches VK_FORMAT_R8G8B8A8_SRGB.
  const Count byte_count = pixels.get_size() * 4;
  auto source_bytes = Core::Data::cast<const Bits_8>(pixels.get_data());
  Bits_8* destination_bytes = static_cast<Bits_8*>(mapped);
  for (Count i = 0; i < byte_count; i++) {
    destination_bytes[i] = source_bytes[i];
  }
  vkUnmapMemory(ctx.get_device(), staging_memory);

  // Device-local image.
  VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_create_info.extent = {width, height, 1};
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  require_success(
      vkCreateImage(
          ctx.get_device(), &image_create_info, nullptr, &texture.image),
      "Vulkan: Failed to create texture image."_view);

  VkMemoryRequirements image_requirements = {};
  vkGetImageMemoryRequirements(
      ctx.get_device(), texture.image, &image_requirements);

  texture.memory = allocate_memory(
      ctx, image_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  require_success(
      vkBindImageMemory(ctx.get_device(), texture.image, texture.memory, 0),
      "Vulkan: Failed to bind texture image memory."_view);

  VkCommandBuffer command_buffer = ctx.begin_immediate_commands();
  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VkAccessFlags2(0),
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

  VkBufferImageCopy region = {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(
      command_buffer, staging, texture.image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  ctx.submit_immediate_commands(command_buffer);

  vkDestroyBuffer(ctx.get_device(), staging, nullptr);
  vkFreeMemory(ctx.get_device(), staging_memory, nullptr);

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = texture.image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  require_success(
      vkCreateImageView(
          ctx.get_device(), &view_info, nullptr, &texture.image_view),
      "Vulkan: Failed to create texture image view."_view);

  VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  require_success(
      vkCreateSampler(
          ctx.get_device(), &sampler_info, nullptr, &texture.sampler),
      "Vulkan: Failed to create texture sampler."_view);

  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = 1;
  layout_info.pBindings = &binding;
  require_success(
      vkCreateDescriptorSetLayout(
          ctx.get_device(), &layout_info, nullptr,
          &texture.descriptor_set_layout),
      "Vulkan: Failed to create texture descriptor set layout."_view);

  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  require_success(
      vkCreateDescriptorPool(
          ctx.get_device(), &pool_info, nullptr, &texture.descriptor_pool),
      "Vulkan: Failed to create texture descriptor pool."_view);

  VkDescriptorSetAllocateInfo set_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  set_info.descriptorPool = texture.descriptor_pool;
  set_info.descriptorSetCount = 1;
  set_info.pSetLayouts = &texture.descriptor_set_layout;
  require_success(
      vkAllocateDescriptorSets(
          ctx.get_device(), &set_info, &texture.descriptor_set),
      "Vulkan: Failed to allocate texture descriptor set."_view);

  VkDescriptorImageInfo descriptor_image_info = {};
  descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptor_image_info.imageView = texture.image_view;
  descriptor_image_info.sampler = texture.sampler;

  VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = texture.descriptor_set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &descriptor_image_info;
  vkUpdateDescriptorSets(ctx.get_device(), 1, &write, 0, nullptr);

  return texture;
}

Texture::~Texture() {
  if (!device) {
    return;
  }
  vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
  vkDestroySampler(device, sampler, nullptr);
  vkDestroyImageView(device, image_view, nullptr);
  vkDestroyImage(device, image, nullptr);
  vkFreeMemory(device, memory, nullptr);
}

Texture::Texture(Texture&& other) noexcept
    : device(other.device),
      image(other.image),
      memory(other.memory),
      image_view(other.image_view),
      sampler(other.sampler),
      descriptor_set_layout(other.descriptor_set_layout),
      descriptor_pool(other.descriptor_pool),
      descriptor_set(other.descriptor_set) {
  other.device = VK_NULL_HANDLE;
}

auto Texture::operator=(Texture&& other) noexcept -> Texture& {
  if (this != &other) {
    this->~Texture();
    device = other.device;
    image = other.image;
    memory = other.memory;
    image_view = other.image_view;
    sampler = other.sampler;
    descriptor_set_layout = other.descriptor_set_layout;
    descriptor_pool = other.descriptor_pool;
    descriptor_set = other.descriptor_set;
    other.device = VK_NULL_HANDLE;
  }
  return *this;
}

auto Texture::get_descriptor_set() const -> VkDescriptorSet {
  return descriptor_set;
}
auto Texture::get_descriptor_set_layout() const -> VkDescriptorSetLayout {
  return descriptor_set_layout;
}
