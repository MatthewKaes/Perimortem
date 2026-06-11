// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/texture.hpp"

using namespace Perimortem::Graphics::Vulkan;

namespace {

auto allocate_memory(
    const Context& context,
    VkMemoryRequirements reqs,
    VkMemoryPropertyFlags props) -> VkDeviceMemory {
  VkMemoryAllocateInfo info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  info.allocationSize = reqs.size;
  info.memoryTypeIndex = context.find_memory_type(reqs.memoryTypeBits, props);
  VkDeviceMemory memory = VK_NULL_HANDLE;
  vkAllocateMemory(context.get_device(), &info, nullptr, &memory);
  return memory;
}

auto transition_image_layout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags2 src_stage,
    VkAccessFlags2 src_access,
    VkPipelineStageFlags2 dst_stage,
    VkAccessFlags2 dst_access) -> void {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = src_stage;
  barrier.srcAccessMask = src_access;
  barrier.dstStageMask = dst_stage;
  barrier.dstAccessMask = dst_access;
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

  VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmd, &dep);
}

}  // namespace

auto Texture::create(const Context& context, const Graphics::Image& source)
    -> Texture {
  Texture tex;
  tex.device = context.get_device();

  const Bits_32 width = source.get_width();
  const Bits_32 height = source.get_height();
  const VkDeviceSize image_size = VkDeviceSize(width) * height * 4;

  // Staging buffer: host-visible, host-coherent.
  VkBufferCreateInfo buf_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buf_info.size = image_size;
  buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer staging = VK_NULL_HANDLE;
  vkCreateBuffer(context.get_device(), &buf_info, nullptr, &staging);

  VkMemoryRequirements staging_reqs = {};
  vkGetBufferMemoryRequirements(context.get_device(), staging, &staging_reqs);

  constexpr VkMemoryPropertyFlags host_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkDeviceMemory staging_memory =
      allocate_memory(context, staging_reqs, host_flags);
  vkBindBufferMemory(context.get_device(), staging, staging_memory, 0);

  void* mapped = nullptr;
  vkMapMemory(context.get_device(), staging_memory, 0, image_size, 0, &mapped);
  const auto pixels = source.get_pixels();
  // Pixel is 4 bytes RGBA — layout matches VK_FORMAT_R8G8B8A8_SRGB.
  const Count byte_count = pixels.get_size() * 4;
  auto src = Core::Data::cast<const Bits_8>(pixels.get_data());
  Bits_8* dst = static_cast<Bits_8*>(mapped);
  for (Count i = 0; i < byte_count; i++) {
    dst[i] = src[i];
  }
  vkUnmapMemory(context.get_device(), staging_memory);

  // Device-local image.
  VkImageCreateInfo img_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  img_info.imageType = VK_IMAGE_TYPE_2D;
  img_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  img_info.extent = {width, height, 1};
  img_info.mipLevels = 1;
  img_info.arrayLayers = 1;
  img_info.samples = VK_SAMPLE_COUNT_1_BIT;
  img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  img_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vkCreateImage(context.get_device(), &img_info, nullptr, &tex.image);

  VkMemoryRequirements img_reqs = {};
  vkGetImageMemoryRequirements(context.get_device(), tex.image, &img_reqs);

  tex.memory =
      allocate_memory(context, img_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkBindImageMemory(context.get_device(), tex.image, tex.memory, 0);

  // Upload: transition → copy → transition.
  context.submit_immediate([&](VkCommandBuffer cmd) {
    transition_image_layout(
        cmd, tex.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VkAccessFlags2(0),
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(
        cmd, staging, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &region);

    transition_image_layout(
        cmd, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  });

  vkDestroyBuffer(context.get_device(), staging, nullptr);
  vkFreeMemory(context.get_device(), staging_memory, nullptr);

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = tex.image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  vkCreateImageView(context.get_device(), &view_info, nullptr, &tex.image_view);

  VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  vkCreateSampler(context.get_device(), &sampler_info, nullptr, &tex.sampler);

  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = 1;
  layout_info.pBindings = &binding;
  vkCreateDescriptorSetLayout(
      context.get_device(), &layout_info, nullptr, &tex.descriptor_set_layout);

  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  vkCreateDescriptorPool(
      context.get_device(), &pool_info, nullptr, &tex.descriptor_pool);

  VkDescriptorSetAllocateInfo set_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  set_info.descriptorPool = tex.descriptor_pool;
  set_info.descriptorSetCount = 1;
  set_info.pSetLayouts = &tex.descriptor_set_layout;
  vkAllocateDescriptorSets(
      context.get_device(), &set_info, &tex.descriptor_set);

  VkDescriptorImageInfo image_info = {};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = tex.image_view;
  image_info.sampler = tex.sampler;

  VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = tex.descriptor_set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &image_info;
  vkUpdateDescriptorSets(context.get_device(), 1, &write, 0, nullptr);

  return tex;
}

Texture::~Texture() {
  if (!device) {
    return;
  }
  vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
  vkDestroySampler(device, sampler, nullptr);
  vkDestroyImageView(device, image_view, nullptr);
  vkFreeMemory(device, memory, nullptr);
  vkDestroyImage(device, image, nullptr);
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
