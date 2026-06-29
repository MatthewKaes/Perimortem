// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/window.hpp"

#include <vulkan/vulkan.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/window.hpp"

#include "perimortem/graphics/vulkan/context.hpp"
#include "perimortem/graphics/vulkan/shader_program.hpp"
#include "perimortem/graphics/vulkan/swapchain.hpp"
#include "perimortem/graphics/vulkan/texture.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
namespace System = Perimortem::System;

constexpr Count max_constant_bytes = 256;
constexpr Count frames_in_flight = 2;

namespace Perimortem::Graphics {

static thread_local Scene* active_scene = nullptr;

}  // namespace Perimortem::Graphics

class Frame {
 public:
  VkCommandBuffer cmd = VK_NULL_HANDLE;
  VkSemaphore image_available = VK_NULL_HANDLE;
  VkSemaphore render_finished = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
};

class Sprite2DResource {
 public:
  auto initialize(const Vulkan::Context& context, const Image& source_image)
      -> void {
    texture = Vulkan::Texture::create(context, source_image);
    set_size_pixels(
        Real_32(source_image.get_width()), Real_32(source_image.get_height()));
  }

  auto set_shader(
      const Vulkan::Context& context,
      const Vulkan::Swapchain& swapchain,
      const Ttx::Shader& source_shader) -> void {
    if (!source_shader.program) {
      Diagnostics::Log::fatal("Graphics::Window: Invalid 2D shader."_view);
    }

    shader = &source_shader;
    rebuild_shader(context, swapchain);
  }

  auto set_position(Real_32 x, Real_32 y) -> void {
    position[0] = x;
    position[1] = y;
  }

  auto set_size_pixels(Real_32 width, Real_32 height) -> void {
    size_pixels[0] = width;
    size_pixels[1] = height;
  }

  auto set_alpha(Real_32 value) -> void { alpha = value; }

  auto rebuild_shader(
      const Vulkan::Context& context,
      const Vulkan::Swapchain& swapchain) -> void {
    if (!shader || !shader->program) {
      Diagnostics::Log::fatal("Graphics::Window: Cannot build 2D shader."_view);
    }

    Static::Vector<VkDescriptorSetLayout, 1> descriptor_set_layouts(
        texture.get_descriptor_set_layout());
    shader_program = Vulkan::ShaderProgram::create(
        context.get_device(), swapchain.get_format(), *shader->program,
        descriptor_set_layouts);
  }

  auto has_shader() const -> Bool { return shader != nullptr; }

  auto render(VkCommandBuffer cmd, Bits_32 width, Bits_32 height) -> void {
    if (!shader) {
      return;
    }

    write_shader_constants(width, height);
    shader_program.bind(cmd);
    shader_program.bind_descriptor_set(cmd, texture.get_descriptor_set());
    if (constant_size > 0) {
      shader_program.push_constants(
          cmd, constant_bytes.get_data(), constant_size);
    }
    shader_program.draw(cmd);
  }

 private:
  static auto push_constant_size(const Ttx::ShaderProgramInfo& shader)
      -> Count {
    Count result = 0;
    for (Count i = 0; i < shader.push_constant_count; i++) {
      const Count range_end =
          shader.push_constants[i].offset + shader.push_constants[i].size;
      if (range_end > result) {
        result = range_end;
      }
    }

    return result;
  }

  template <typename ValueType>
  static auto write_object_property(
      Bits_8* constants,
      Count constant_size,
      const Ttx::ShaderObjectPropertyBinding& binding,
      const ValueType* value,
      Count count = 1) -> void {
    const Count byte_count = sizeof(ValueType) * count;
    if (binding.size != byte_count ||
        binding.offset + binding.size > constant_size) {
      Diagnostics::Log::fatal(
          "Graphics::Window: Invalid 2D shader object property."_view);
    }

    Data::copy(constants + binding.offset, value, count);
  }

  auto set_constant_size(Count size) -> void {
    if (size > max_constant_bytes) {
      Diagnostics::Log::fatal(
          "Graphics::Window: 2D shader constants are too large."_view);
    }

    constant_size = size;
  }

  auto write_shader_constants(Bits_32 width, Bits_32 height) -> void {
    if (!shader) {
      return;
    }

    const Ttx::ShaderProgramInfo& program = *shader->program;
    if (program.push_constant_count == 0) {
      set_constant_size(0);
      return;
    }
    if (!program.push_constants) {
      Diagnostics::Log::fatal(
          "Graphics::Window: Invalid 2D shader constants."_view);
    }
    if (program.object_property_count > 0 && !program.object_properties) {
      Diagnostics::Log::fatal(
          "Graphics::Window: Invalid 2D shader object properties."_view);
    }
    if (width == 0 || height == 0) {
      Diagnostics::Log::fatal(
          "Graphics::Window: Cannot render to zero-sized extent."_view);
    }

    set_constant_size(push_constant_size(program));
    Data::set(constant_bytes.get_data(), 0, constant_size);

    Static::Vector<Real_32, 2> half_size(
        size_pixels[0] / Real_32(width), size_pixels[1] / Real_32(height));

    for (Count i = 0; i < program.object_property_count; i++) {
      const Ttx::ShaderObjectPropertyBinding& binding =
          program.object_properties[i];
      switch (binding.property) {
      case Ttx::ShaderObjectProperty::Position:
        write_object_property(
            constant_bytes.get_data(), constant_size, binding,
            position.get_data(), 2);
        break;
      case Ttx::ShaderObjectProperty::HalfSize:
        write_object_property(
            constant_bytes.get_data(), constant_size, binding,
            half_size.get_data(), 2);
        break;
      case Ttx::ShaderObjectProperty::Alpha:
        write_object_property(
            constant_bytes.get_data(), constant_size, binding, &alpha);
        break;
      }
    }
  }

  Vulkan::Texture texture;
  Vulkan::ShaderProgram shader_program;
  const Ttx::Shader* shader = nullptr;
  Static::Vector<Real_32, 2> position;
  Static::Vector<Real_32, 2> size_pixels;
  Real_32 alpha = 1.0f;
  alignas(16) Static::Bytes<max_constant_bytes> constant_bytes;
  Count constant_size = 0;
};

class WindowState {
 public:
  WindowState(Bits_32 width, Bits_32 height, const char* title)
      : window(width, height, title),
        context(Vulkan::Context::create(window)),
        swapchain(
            Vulkan::Swapchain::create(
                context,
                window.get_logical_width() * window.get_scale(),
                window.get_logical_height() * window.get_scale())) {
    allocate_frames();
    refresh_swapchain_images();
  }

  ~WindowState() {
    wait_idle();
    destroy_frames();
  }

  auto create_sprite_2d(const Image& image) -> Count {
    if (sprite_2d_count >= Window::max_sprites_2d) {
      Diagnostics::Log::fatal("Graphics::Window: Too many 2D sprites."_view);
    }

    const Count result = sprite_2d_count++;
    sprites_2d[result].initialize(context, image);
    return result;
  }

  auto set_sprite_2d_shader(Count sprite_index, const Ttx::Shader& shader)
      -> void {
    sprites_2d[sprite_index].set_shader(context, swapchain, shader);
  }

  auto set_sprite_2d_position(Count sprite_index, Real_32 x, Real_32 y)
      -> void {
    sprites_2d[sprite_index].set_position(x, y);
  }

  auto set_sprite_2d_size(Count sprite_index, Real_32 width, Real_32 height)
      -> void {
    sprites_2d[sprite_index].set_size_pixels(width, height);
  }

  auto set_sprite_2d_alpha(Count sprite_index, Real_32 value) -> void {
    sprites_2d[sprite_index].set_alpha(value);
  }

  auto poll_events() -> Bool {
    if (!window.poll_events()) {
      return False;
    }

    if (window.get_needs_resize()) {
      resize(
          window.get_logical_width() * window.get_scale(),
          window.get_logical_height() * window.get_scale());
      window.clear_resize();
    }

    return True;
  }

  auto render() -> void {
    Frame& frame = frames[current_frame];
    vkWaitForFences(context.get_device(), 1, &frame.fence, VK_TRUE, UINT64_MAX);

    const Bits_32 image_index =
        swapchain.acquire_next_image(frame.image_available);
    if (image_index == UINT32_MAX) {
      return;
    }

    vkResetFences(context.get_device(), 1, &frame.fence);
    vkResetCommandBuffer(frame.cmd, 0);

    record_frame(
        frame.cmd, swapchain_images[image_index],
        swapchain.get_image_view(image_index));

    constexpr VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.image_available;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame.cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.render_finished;
    require_success(
        vkQueueSubmit(context.get_graphics_queue(), 1, &submit, frame.fence),
        "Graphics::Window: Failed to submit frame."_view);

    const VkSwapchainKHR sc = swapchain.get_swapchain();
    VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &frame.render_finished;
    present.swapchainCount = 1;
    present.pSwapchains = &sc;
    present.pImageIndices = &image_index;

    const VkResult present_result =
        vkQueuePresentKHR(context.get_graphics_queue(), &present);
    if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR &&
        present_result != VK_ERROR_OUT_OF_DATE_KHR) {
      Diagnostics::Log::fatal(
          "Graphics::Window: Failed to present frame."_view);
    }

    current_frame = (current_frame + 1) % frames_in_flight;
  }

  auto wait_idle() -> void {
    if (context.get_device()) {
      vkDeviceWaitIdle(context.get_device());
    }
  }

  auto get_width() -> Bits_32 { return swapchain.get_extent().width; }

  auto get_height() -> Bits_32 { return swapchain.get_extent().height; }

 private:
  static auto require_success(VkResult result, View::Bytes message) -> void {
    if (result != VK_SUCCESS) {
      Diagnostics::Log::fatal(message);
    }
  }

  auto resize(Bits_32 width, Bits_32 height) -> void {
    wait_idle();
    const VkFormat old_format = swapchain.get_format();
    swapchain.recreate(context, width, height);
    refresh_swapchain_images();

    if (swapchain.get_format() != old_format) {
      recreate_sprite_2d_shaders();
    }
  }

  auto allocate_frames() -> void {
    VkCommandBufferAllocateInfo cmd_alloc = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = context.get_command_pool();
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    for (Count i = 0; i < frames_in_flight; i++) {
      require_success(
          vkAllocateCommandBuffers(
              context.get_device(), &cmd_alloc, &frames[i].cmd),
          "Graphics::Window: Failed to allocate command buffer."_view);

      VkSemaphoreCreateInfo sem_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      require_success(
          vkCreateSemaphore(
              context.get_device(), &sem_info, nullptr,
              &frames[i].image_available),
          "Graphics::Window: Failed to create image-available semaphore."_view);
      require_success(
          vkCreateSemaphore(
              context.get_device(), &sem_info, nullptr,
              &frames[i].render_finished),
          "Graphics::Window: Failed to create render-finished semaphore."_view);

      VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      require_success(
          vkCreateFence(
              context.get_device(), &fence_info, nullptr, &frames[i].fence),
          "Graphics::Window: Failed to create frame fence."_view);
    }
  }

  auto destroy_frames() -> void {
    for (Count i = 0; i < frames_in_flight; i++) {
      if (frames[i].fence) {
        vkDestroyFence(context.get_device(), frames[i].fence, nullptr);
      }
      if (frames[i].render_finished) {
        vkDestroySemaphore(
            context.get_device(), frames[i].render_finished, nullptr);
      }
      if (frames[i].image_available) {
        vkDestroySemaphore(
            context.get_device(), frames[i].image_available, nullptr);
      }

      frames[i] = {};
    }
  }

  auto refresh_swapchain_images() -> void {
    Bits_32 image_total = swapchain.get_image_count();
    require_success(
        vkGetSwapchainImagesKHR(
            context.get_device(), swapchain.get_swapchain(), &image_total,
            swapchain_images.get_data()),
        "Graphics::Window: Failed to get swapchain images."_view);
  }

  auto recreate_sprite_2d_shaders() -> void {
    for (Count i = 0; i < sprite_2d_count; i++) {
      if (sprites_2d[i].has_shader()) {
        sprites_2d[i].rebuild_shader(context, swapchain);
      }
    }
  }

  auto record_frame(
      VkCommandBuffer cmd,
      VkImage swapchain_image,
      VkImageView swapchain_image_view) -> void {
    VkCommandBufferBeginInfo begin = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require_success(
        vkBeginCommandBuffer(cmd, &begin),
        "Graphics::Window: Failed to begin command buffer."_view);

    VkImageMemoryBarrier2 to_attachment = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    to_attachment.srcStageMask =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_attachment.srcAccessMask = VkAccessFlags2(0);
    to_attachment.dstStageMask =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_attachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_attachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_attachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_attachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_attachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_attachment.image = swapchain_image;
    to_attachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo dep_attach = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep_attach.imageMemoryBarrierCount = 1;
    dep_attach.pImageMemoryBarriers = &to_attachment;
    vkCmdPipelineBarrier2(cmd, &dep_attach);

    const VkExtent2D extent = swapchain.get_extent();

    VkRenderingAttachmentInfo color_attachment = {
      VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView = swapchain_image_view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderingInfo rendering = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea = {{0, 0}, extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &color_attachment;

    vkCmdBeginRendering(cmd, &rendering);

    const VkViewport viewport = {
      0.0f, 0.0f, Real_32(extent.width), Real_32(extent.height), 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const VkRect2D scissor = {{0, 0}, extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (Count i = 0; i < sprite_2d_count; i++) {
      sprites_2d[i].render(cmd, extent.width, extent.height);
    }

    vkCmdEndRendering(cmd);

    VkImageMemoryBarrier2 to_present = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    to_present.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_present.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_present.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    to_present.dstAccessMask = VkAccessFlags2(0);
    to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present.image = swapchain_image;
    to_present.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo dep_present = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep_present.imageMemoryBarrierCount = 1;
    dep_present.pImageMemoryBarriers = &to_present;
    vkCmdPipelineBarrier2(cmd, &dep_present);

    require_success(
        vkEndCommandBuffer(cmd),
        "Graphics::Window: Failed to end command buffer."_view);
  }

  System::Window window;
  Vulkan::Context context;
  Vulkan::Swapchain swapchain;
  Static::Vector<Sprite2DResource, Window::max_sprites_2d> sprites_2d;
  Count sprite_2d_count = 0;
  Static::Vector<Frame, frames_in_flight> frames;
  Static::Vector<VkImage, Vulkan::Swapchain::max_images> swapchain_images;
  Count current_frame = 0;
};

namespace Perimortem::Graphics {

Window::Runtime::Runtime(Bits_32 width, Bits_32 height, const char* title)
    : width(width), height(height), title(title) {
  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&command_ready, nullptr);
  pthread_cond_init(&command_finished, nullptr);
  pthread_cond_init(&runtime_ready, nullptr);
}

Window::Runtime::~Runtime() {
  pthread_cond_destroy(&runtime_ready);
  pthread_cond_destroy(&command_finished);
  pthread_cond_destroy(&command_ready);
  pthread_mutex_destroy(&mutex);
}

auto Window::Runtime::operator()() -> void {
  WindowState state(width, height, title);

  pthread_mutex_lock(&mutex);
  ready = True;
  pthread_cond_signal(&runtime_ready);
  pthread_mutex_unlock(&mutex);

  Bool running = True;
  while (running) {
    Command command = wait_for_command();
    switch (command.kind) {
    case CommandKind::Stop:
      running = False;
      break;
    case CommandKind::CreateSprite2D:
      command.sprite_index = state.create_sprite_2d(*command.image);
      break;
    case CommandKind::SetSprite2DShader:
      state.set_sprite_2d_shader(command.sprite_index, *command.shader);
      break;
    case CommandKind::SetSprite2DPosition:
      state.set_sprite_2d_position(command.sprite_index, command.x, command.y);
      break;
    case CommandKind::SetSprite2DSize:
      state.set_sprite_2d_size(command.sprite_index, command.x, command.y);
      break;
    case CommandKind::SetSprite2DAlpha:
      state.set_sprite_2d_alpha(command.sprite_index, command.x);
      break;
    case CommandKind::PollEvents:
      command.bool_result = state.poll_events();
      break;
    case CommandKind::Render:
      state.render();
      break;
    case CommandKind::WaitIdle:
      state.wait_idle();
      break;
    case CommandKind::GetWidth:
      command.bits_32_result = state.get_width();
      break;
    case CommandKind::GetHeight:
      command.bits_32_result = state.get_height();
      break;
    }
    finish_command(command);
  }
}

auto Window::Runtime::wait_until_ready() -> void {
  pthread_mutex_lock(&mutex);
  while (!ready) {
    pthread_cond_wait(&runtime_ready, &mutex);
  }
  pthread_mutex_unlock(&mutex);
}

auto Window::Runtime::execute(Command& command) -> void {
  pthread_mutex_lock(&mutex);
  while (has_command) {
    pthread_cond_wait(&command_finished, &mutex);
  }

  pending_command = command;
  command_complete = False;
  has_command = True;
  pthread_cond_signal(&command_ready);

  while (!command_complete) {
    pthread_cond_wait(&command_finished, &mutex);
  }

  command = pending_command;
  pthread_mutex_unlock(&mutex);
}

auto Window::Runtime::wait_for_command() -> Command {
  pthread_mutex_lock(&mutex);
  while (!has_command) {
    pthread_cond_wait(&command_ready, &mutex);
  }
  Command command = pending_command;
  pthread_mutex_unlock(&mutex);
  return command;
}

auto Window::Runtime::finish_command(const Command& command) -> void {
  pthread_mutex_lock(&mutex);
  pending_command = command;
  has_command = False;
  command_complete = True;
  pthread_cond_signal(&command_finished);
  pthread_mutex_unlock(&mutex);
}

auto Sprite2D::create(const Image& image) -> Sprite2D& {
  return Scene::current().create_sprite_2d(image);
}

auto Sprite2D::set_shader(const Ttx::Shader& shader) -> void {
  window->set_sprite_2d_shader(index, shader);
}

auto Sprite2D::set_position(Real_32 x, Real_32 y) -> void {
  window->set_sprite_2d_position(index, x, y);
}

auto Sprite2D::set_size_pixels(Real_32 width, Real_32 height) -> void {
  window->set_sprite_2d_size(index, width, height);
}

auto Sprite2D::set_alpha(Real_32 value) -> void {
  window->set_sprite_2d_alpha(index, value);
}

auto Sprite2D::initialize(Window* owner, Count sprite_index) -> void {
  window = owner;
  index = sprite_index;
}

Window::Window(Bits_32 width, Bits_32 height, const char* title)
    : runtime(width, height, title) {
  worker = Core::Thread::Worker::start("graphics.window"_view, runtime);
  runtime.wait_until_ready();
}

Window::~Window() {
  Command command;
  command.kind = CommandKind::Stop;
  runtime.execute(command);
  worker.join();
}

auto Window::poll_events() -> Bool {
  Command command;
  command.kind = CommandKind::PollEvents;
  runtime.execute(command);
  return command.bool_result;
}

auto Window::render() -> void {
  Command command;
  command.kind = CommandKind::Render;
  runtime.execute(command);
}

auto Window::wait_idle() -> void {
  Command command;
  command.kind = CommandKind::WaitIdle;
  runtime.execute(command);
}

auto Window::get_width() -> Bits_32 {
  Command command;
  command.kind = CommandKind::GetWidth;
  runtime.execute(command);
  return command.bits_32_result;
}

auto Window::get_height() -> Bits_32 {
  Command command;
  command.kind = CommandKind::GetHeight;
  runtime.execute(command);
  return command.bits_32_result;
}

auto Window::create_sprite_2d(const Image& image) -> Sprite2D& {
  if (sprite_2d_count >= max_sprites_2d) {
    Diagnostics::Log::fatal("Graphics::Window: Too many 2D sprites."_view);
  }

  Command command;
  command.kind = CommandKind::CreateSprite2D;
  command.image = &image;
  runtime.execute(command);

  Sprite2D& result = sprites_2d[sprite_2d_count++];
  result.initialize(this, command.sprite_index);
  return result;
}

auto Window::set_sprite_2d_shader(Count sprite_index, const Ttx::Shader& shader)
    -> void {
  Command command;
  command.kind = CommandKind::SetSprite2DShader;
  command.sprite_index = sprite_index;
  command.shader = &shader;
  runtime.execute(command);
}

auto Window::set_sprite_2d_position(Count sprite_index, Real_32 x, Real_32 y)
    -> void {
  Command command;
  command.kind = CommandKind::SetSprite2DPosition;
  command.sprite_index = sprite_index;
  command.x = x;
  command.y = y;
  runtime.execute(command);
}

auto Window::set_sprite_2d_size(
    Count sprite_index,
    Real_32 width,
    Real_32 height) -> void {
  Command command;
  command.kind = CommandKind::SetSprite2DSize;
  command.sprite_index = sprite_index;
  command.x = width;
  command.y = height;
  runtime.execute(command);
}

auto Window::set_sprite_2d_alpha(Count sprite_index, Real_32 value) -> void {
  Command command;
  command.kind = CommandKind::SetSprite2DAlpha;
  command.sprite_index = sprite_index;
  command.x = value;
  runtime.execute(command);
}

Scene::Scene(Window& target_window)
    : window(&target_window), previous(active_scene) {
  active_scene = this;
}

Scene::~Scene() {
  if (active_scene == this) {
    active_scene = previous;
  }
}

auto Scene::poll_events() -> Bool {
  return window->poll_events();
}

auto Scene::render() -> void {
  window->render();
}

auto Scene::wait_idle() -> void {
  window->wait_idle();
}

auto Scene::get_width() -> Bits_32 {
  return window->get_width();
}

auto Scene::get_height() -> Bits_32 {
  return window->get_height();
}

auto Scene::current() -> Scene& {
  if (!active_scene) {
    Diagnostics::Log::fatal(
        "Graphics::Scene: No active scene for render object creation."_view);
  }
  return *active_scene;
}

auto Scene::create_sprite_2d(const Image& image) -> Sprite2D& {
  return window->create_sprite_2d(image);
}

}  // namespace Perimortem::Graphics
