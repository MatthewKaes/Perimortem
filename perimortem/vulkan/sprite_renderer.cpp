// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/sprite_renderer.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/graphics/frame/sprite.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

const Core::Object<>::Descriptor Vulkan::SpriteRenderer::cache_descriptor(
    sizeof(CacheEntry),
    alignof(CacheEntry),
    SpriteRenderer::finalize_cache);

Vulkan::SpriteRenderer::SpriteRenderer(
    const Context& context,
    VkFormat color_format,
    Perimortem::Graphics::Frame::Program program,
    Description::Program description)
    : context(context), program(program), description(description) {
  create_descriptor_layout();
  create_vertex_buffer();
  rebuild(color_format);
}

Vulkan::SpriteRenderer::~SpriteRenderer() {
  vkDeviceWaitIdle(context.get_device());
  for (Core::Object<> texture : textures.get_view()) {
    texture.release();
  }
  textures.clear();
  shader = ShaderProgram();
  destroy_vertex_buffer();
  if (descriptor_layout) {
    vkDestroyDescriptorSetLayout(
        context.get_device(), descriptor_layout, nullptr);
  }
}

auto Vulkan::SpriteRenderer::rebuild(VkFormat color_format) -> void {
  View::Vector<VkDescriptorSetLayout> layouts(&descriptor_layout, 1);
  shader = ShaderProgram::create(
      context.get_device(), color_format, description, layouts);
}

auto Vulkan::SpriteRenderer::record(
    VkCommandBuffer command_buffer,
    U32 width,
    U32 height,
    View::Vector<Perimortem::Graphics::Frame::Batch> batches) -> Bool {
  BAIL_IF(!command_buffer || width == 0 || height == 0 || !validate(batches));

  // Admission runs across the complete sequence before command recording. A
  // bad Program or resource therefore cannot leave a partial frame mixed with
  // draws that happened to precede it.
  for (const Perimortem::Graphics::Frame::Batch& batch : batches) {
    const Perimortem::Graphics::Frame::Resource& resource =
        batch.get_resources().get_data()[0];
    const auto* frame = Data::cast<const Perimortem::Graphics::Frame::Sprite>(
        batch.get_inputs().get_data());
    Texture* texture = realize_texture(resource, frame->image_size_pixels);
    BAIL_IF(!texture);

    PushConstants inputs = make_push_constants(batch, width, height);
    shader.bind(command_buffer);
    if (vertex_buffer) {
      constexpr VkDeviceSize vertex_offset = 0;
      vkCmdBindVertexBuffers(
          command_buffer, 0, 1, &vertex_buffer, &vertex_offset);
    }
    shader.bind_descriptor_set(command_buffer, texture->get_descriptor_set());
    shader.push_constants(
        command_buffer,
        View::Bytes(Data::cast<const U8>(&inputs), sizeof(inputs)));
    shader.draw(command_buffer, batch.get_vertex_count());
  }

  sweep_textures();
  return True;
}

auto Vulkan::SpriteRenderer::create_descriptor_layout() -> void {
  auto descriptors = description.descriptors;
  if (descriptors.get_size() != 1 || descriptors.get_data()[0].set != 0 ||
      descriptors.get_data()[0].slot != 0) {
    Diagnostics::Log::fatal(
        "Vulkan: Sprite Program requires one image descriptor at set 0 binding 0."_view);
  }

  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.bindingCount = 1;
  info.pBindings = &binding;
  if (vkCreateDescriptorSetLayout(
          context.get_device(), &info, nullptr, &descriptor_layout) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal(
        "Vulkan: Failed to create the Sprite descriptor layout."_view);
  }
}

auto Vulkan::SpriteRenderer::create_vertex_buffer() -> void {
  struct Vertex {
    R32 position[2];
    R32 texture_uv[2];
  };
  static constexpr Vertex vertices[] = {
    {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
  };
  auto inputs = description.vertex_inputs;
  if (inputs.is_empty()) {
    return;
  }
  if (inputs.get_size() != 2 || inputs.get_data()[0].location != 0 ||
      inputs.get_data()[0].components != 2 ||
      inputs.get_data()[0].offset != 0 ||
      inputs.get_data()[0].stride != sizeof(Vertex) ||
      inputs.get_data()[1].location != 1 ||
      inputs.get_data()[1].components != 2 ||
      inputs.get_data()[1].offset != sizeof(R32) * 2 ||
      inputs.get_data()[1].stride != sizeof(Vertex)) {
    Diagnostics::Log::fatal(
        "Vulkan: Sprite Program has an incompatible vertex layout."_view);
  }

  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = sizeof(vertices);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(
          context.get_device(), &buffer_info, nullptr, &vertex_buffer) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to create vertex buffer."_view);
  }

  VkMemoryRequirements requirements = {};
  vkGetBufferMemoryRequirements(
      context.get_device(), vertex_buffer, &requirements);
  constexpr VkMemoryPropertyFlags properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  U32 memory_type =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (memory_type == UINT32_MAX) {
    Diagnostics::Log::fatal("Vulkan: No compatible vertex memory."_view);
  }
  VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocation.allocationSize = requirements.size;
  allocation.memoryTypeIndex = memory_type;
  if (vkAllocateMemory(
          context.get_device(), &allocation, nullptr, &vertex_memory) !=
          VK_SUCCESS ||
      vkBindBufferMemory(
          context.get_device(), vertex_buffer, vertex_memory, 0) !=
          VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to allocate vertex memory."_view);
  }

  void* mapped = nullptr;
  if (vkMapMemory(
          context.get_device(), vertex_memory, 0, sizeof(vertices), 0,
          &mapped) != VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to map vertex memory."_view);
  }
  auto* destination = Core::Data::cast<R32>(mapped);
  const auto* source = Core::Data::cast<const R32>(vertices);
  Count word_count = sizeof(vertices);
  word_count /= sizeof(R32);
  for (Count index = 0; index < word_count; index++) {
    destination[index] = source[index];
  }
  vkUnmapMemory(context.get_device(), vertex_memory);
}

auto Vulkan::SpriteRenderer::destroy_vertex_buffer() -> void {
  if (vertex_buffer) {
    vkDestroyBuffer(context.get_device(), vertex_buffer, nullptr);
    vertex_buffer = VK_NULL_HANDLE;
  }
  if (vertex_memory) {
    vkFreeMemory(context.get_device(), vertex_memory, nullptr);
    vertex_memory = VK_NULL_HANDLE;
  }
}

auto Vulkan::SpriteRenderer::validate(
    View::Vector<Perimortem::Graphics::Frame::Batch> batches) const -> Bool {
  for (const Perimortem::Graphics::Frame::Batch& batch : batches) {
    auto resources = batch.get_resources();
    BAIL_IF(
        !(batch.get_program() == program) || resources.get_size() != 1 ||
        batch.get_inputs().get_size() !=
            sizeof(Perimortem::Graphics::Frame::Sprite) ||
        batch.get_vertex_count() != 6);
    const auto* frame = Data::cast<const Perimortem::Graphics::Frame::Sprite>(
        batch.get_inputs().get_data());
    Count required = Count(frame->image_size_pixels.width) *
                     Count(frame->image_size_pixels.height) *
                     sizeof(Perimortem::Graphics::Pixel);
    BAIL_IF(
        frame->image_size_pixels.width == 0 ||
        frame->image_size_pixels.height == 0 ||
        resources.get_data()[0].get_capacity() < required);
  }
  return True;
}

auto Vulkan::SpriteRenderer::find_texture(
    const Perimortem::Graphics::Frame::Resource& resource,
    Perimortem::Graphics::Size2D size_pixels) -> Texture* {
  for (Count index = 0; index < textures.get_size(); index++) {
    CacheEntry& entry = get_cache_entry(textures[index]);
    if (entry.resource.get_payload() == resource.get_payload() &&
        entry.size_pixels.width == size_pixels.width &&
        entry.size_pixels.height == size_pixels.height) {
      return &entry.texture;
    }
  }
  return nullptr;
}

auto Vulkan::SpriteRenderer::realize_texture(
    const Perimortem::Graphics::Frame::Resource& resource,
    Perimortem::Graphics::Size2D size_pixels) -> Texture* {
  Texture* retained = find_texture(resource, size_pixels);
  if (retained) {
    return retained;
  }

  // The cache retains the same pixel storage as the frame. Vulkan storage can
  // therefore survive several submissions without copying pixels or teaching
  // Graphics about device lifetime.
  Core::Object<> storage = Core::Object<>::create(cache_descriptor);
  new (storage.get_payload(), Core::Placement::Construct) CacheEntry();
  CacheEntry& entry = get_cache_entry(storage);
  entry.resource = resource;
  entry.size_pixels = size_pixels;
  entry.texture =
      Texture::create(context, resource, size_pixels, descriptor_layout);
  Core::Object<>& inserted =
      textures.emplace(static_cast<Core::Object<>&&>(storage));
  return &get_cache_entry(inserted).texture;
}

auto Vulkan::SpriteRenderer::sweep_textures() -> void {
  Count index = 0;
  while (index < textures.get_size()) {
    CacheEntry& entry = get_cache_entry(textures[index]);
    // One reservation means the cache is the final owner. Releasing it here
    // makes the Texture lifetime follow real Image use without a registry or a
    // separate invalidation message.
    if (entry.resource.get_reservations() == 1) {
      textures[index].release();
      textures.remove(index);
    } else {
      index++;
    }
  }
}

auto Vulkan::SpriteRenderer::finalize_cache(U8* payload) -> void {
  Data::cast<CacheEntry>(payload)->~CacheEntry();
}

auto Vulkan::SpriteRenderer::get_cache_entry(Core::Object<> object)
    -> CacheEntry& {
  return *Data::cast<CacheEntry>(object.get_payload());
}

auto Vulkan::SpriteRenderer::make_push_constants(
    const Perimortem::Graphics::Frame::Batch& batch,
    U32 width,
    U32 height) -> PushConstants {
  const auto& transform = batch.get_transform();
  const auto* frame = Data::cast<const Perimortem::Graphics::Frame::Sprite>(
      batch.get_inputs().get_data());
  const R64 sprite_width = frame->size_pixels.width;
  const R64 sprite_height = frame->size_pixels.height;

  // Graphics keeps authored values in R64 and pixel coordinates. Vulkan owns
  // the target conversion to the R32 push layout and the current framebuffer
  // extent expected by this Program.
  PushConstants inputs = {};
  inputs.transform_x[0] = R32(transform.get_xx() * sprite_width);
  inputs.transform_x[1] = R32(transform.get_xy() * sprite_height);
  inputs.transform_x[2] = R32(transform.get_x());
  inputs.transform_x[3] = R32(width);
  inputs.transform_y[0] = R32(transform.get_yx() * sprite_width);
  inputs.transform_y[1] = R32(transform.get_yy() * sprite_height);
  inputs.transform_y[2] = R32(transform.get_y());
  inputs.transform_y[3] = R32(height);
  inputs.tone[0] = R32(frame->tone.red);
  inputs.tone[1] = R32(frame->tone.green);
  inputs.tone[2] = R32(frame->tone.blue);
  inputs.tone[3] = R32(frame->tone.alpha);
  return inputs;
}
