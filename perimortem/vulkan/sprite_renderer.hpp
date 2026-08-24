// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/batch.hpp"
#include "perimortem/vulkan/context.hpp"
#include "perimortem/vulkan/description/program.hpp"
#include "perimortem/vulkan/shader_program.hpp"
#include "perimortem/vulkan/texture.hpp"

namespace Perimortem::Vulkan {

// SpriteRenderer realizes one compiled Graphics Program for a selected Vulkan
// device. It owns target resources keyed by the retained Image identity while
// each frame continues to own ordering, transforms, and draw inputs.
class SpriteRenderer {
 public:
  SpriteRenderer(
      const Context& context,
      VkFormat color_format,
      Perimortem::Graphics::Frame::Program program,
      Description::Program description);
  ~SpriteRenderer();
  SpriteRenderer(const SpriteRenderer&) = delete;
  SpriteRenderer(SpriteRenderer&&) = delete;
  auto operator=(const SpriteRenderer&) -> SpriteRenderer& = delete;
  auto operator=(SpriteRenderer&&) -> SpriteRenderer& = delete;

  auto rebuild(VkFormat color_format) -> void;
  auto record(
      VkCommandBuffer command_buffer,
      U32 width,
      U32 height,
      Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch>
          batches) -> Bool;

 private:
  class CacheEntry {
   public:
    Perimortem::Graphics::Frame::Resource resource;
    Perimortem::Graphics::Size2D size_pixels;
    Texture texture;
  };

  struct PushConstants {
    R32 transform_x[4];
    R32 transform_y[4];
    R32 tone[4];
  };
  static_assert(sizeof(PushConstants) == sizeof(R32) * 12);

  auto create_descriptor_layout() -> void;
  auto validate(
      Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch>
          batches) const -> Bool;
  auto find_texture(
      const Perimortem::Graphics::Frame::Resource& resource,
      Perimortem::Graphics::Size2D size_pixels) -> Texture*;
  auto realize_texture(
      const Perimortem::Graphics::Frame::Resource& resource,
      Perimortem::Graphics::Size2D size_pixels) -> Texture*;
  auto sweep_textures() -> void;
  static auto finalize_cache(U8* payload) -> void;
  static auto get_cache_entry(Perimortem::Core::Object<> object) -> CacheEntry&;
  static auto make_push_constants(
      const Perimortem::Graphics::Frame::Batch& batch,
      U32 width,
      U32 height) -> PushConstants;

  const Context& context;
  Perimortem::Graphics::Frame::Program program;
  Description::Program description;
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  ShaderProgram shader;
  Perimortem::Memory::Dynamic::Vector<Perimortem::Core::Object<>> textures;
  static const Perimortem::Core::Object<>::Descriptor cache_descriptor;
};

}  // namespace Perimortem::Vulkan
