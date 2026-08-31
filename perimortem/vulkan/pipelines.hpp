// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.h"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/batch.hpp"
#include "perimortem/vulkan/context.hpp"
#include "perimortem/vulkan/description/program.hpp"
#include "perimortem/vulkan/shader_program.hpp"
#include "perimortem/vulkan/texture.hpp"

namespace Perimortem::Vulkan {

// Pipelines realizes each generated Program and draw state pair selected by a
// frame. It caches target resources by shared Image identity plus sampling
// policy while frame Batches keep exact program selection, fixed state, and
// parameter bytes.
class Pipelines {
 public:
  Pipelines(
      const Context& context,
      VkFormat color_format,
      Perimortem::Core::View::Vector<Description::Program> descriptions);
  ~Pipelines();
  Pipelines(const Pipelines&) = delete;
  Pipelines(Pipelines&&) = delete;
  auto operator=(const Pipelines&) -> Pipelines& = delete;
  auto operator=(Pipelines&&) -> Pipelines& = delete;

  auto rebuild(VkFormat color_format) -> void;
  auto record(
      VkCommandBuffer command_buffer,
      U32 width,
      U32 height,
      Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch>
          batches) -> Bool;

 private:
  class ImageCacheEntry {
   public:
    Perimortem::Graphics::Frame::Resource resource;
    TextureImage image;
  };

  class TextureCacheEntry {
   public:
    Perimortem::Graphics::Frame::Resource resource;
    Texture texture;
  };

  class Realization {
   public:
    const Description::Program* description = nullptr;
    Perimortem::Graphics::Frame::Pipeline pipeline;
    ShaderProgram shader;
  };

  auto validate_descriptions() const -> void;
  auto create_descriptor_layout() -> void;
  auto create_vertex_buffer() -> void;
  auto destroy_vertex_buffer() -> void;
  auto validate(
      Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch>
          batches) const -> Bool;
  auto find_description(const U8* locator) const -> const Description::Program*;
  auto find_realization(
      const U8* locator,
      Perimortem::Graphics::Frame::Pipeline pipeline) -> Realization*;
  auto find_realization(
      const U8* locator,
      Perimortem::Graphics::Frame::Pipeline pipeline) const
      -> const Realization*;
  auto realize_pipeline(const Perimortem::Graphics::Frame::Batch& batch)
      -> Realization*;
  auto find_image(const Perimortem::Graphics::Frame::Resource& resource)
      -> TextureImage*;
  auto realize_image(const Perimortem::Graphics::Frame::Resource& resource)
      -> TextureImage*;
  auto find_texture(const Perimortem::Graphics::Frame::Resource& resource)
      -> Texture*;
  auto realize_texture(const Perimortem::Graphics::Frame::Resource& resource)
      -> Texture*;
  auto make_host_inputs(
      const Description::Program& description,
      const Perimortem::Graphics::Frame::Batch& batch,
      U32 width,
      U32 height) const -> Perimortem::Memory::Dynamic::Bytes;
  auto sweep_textures() -> void;

  static auto finalize_image_cache(U8* payload) -> void;
  static auto finalize_cache(U8* payload) -> void;
  static auto finalize_realization(U8* payload) -> void;
  static auto get_image_cache_entry(U8* object) -> ImageCacheEntry&;
  static auto get_cache_entry(U8* object) -> TextureCacheEntry&;
  static auto get_realization(U8* object) -> Realization&;

  const Context& context;
  Perimortem::Core::View::Vector<Description::Program> descriptions;
  VkFormat color_format = VK_FORMAT_UNDEFINED;
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  VkBuffer vertex_buffer = VK_NULL_HANDLE;
  VkDeviceMemory vertex_memory = VK_NULL_HANDLE;
  Perimortem::Memory::Dynamic::Vector<U8*> realizations;
  Perimortem::Memory::Dynamic::Vector<U8*> images;
  Perimortem::Memory::Dynamic::Vector<U8*> textures;
  static const perimortem_object_descriptor image_cache_descriptor;
  static const perimortem_object_descriptor cache_descriptor;
  static const perimortem_object_descriptor realization_descriptor;
};

}  // namespace Perimortem::Vulkan
