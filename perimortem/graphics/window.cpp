// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/window.hpp"

#include <vulkan/vulkan.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/system/window.hpp"

#include "perimortem/graphics/vulkan/context.hpp"
#include "perimortem/graphics/vulkan/shader_program.hpp"
#include "perimortem/graphics/vulkan/swapchain.hpp"
#include "perimortem/graphics/vulkan/texture.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;

constexpr Count max_host_input_bytes = 256;
constexpr Count frames_in_flight = 2;

namespace Perimortem::Graphics {

static thread_local Scene* active_scene = nullptr;

}  // namespace Perimortem::Graphics

class Frame {
 public:
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  VkSemaphore image_available = VK_NULL_HANDLE;
  VkSemaphore render_finished = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
};

class Sprite2DResource {
 public:
  auto initialize(const Vulkan::Context& ctx, const Image& source_image)
      -> void {
    texture = Vulkan::Texture::create(ctx, source_image);
    set_size_pixels(
        Real_32(source_image.get_width()), Real_32(source_image.get_height()));
  }

  auto set_renderer(
      const Vulkan::Context& ctx,
      const Vulkan::Swapchain& swapchain,
      const Render& source_renderer) -> void {
    if (!source_renderer.is_valid()) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Invalid 2D renderer."_view);
    }

    renderer = &source_renderer;
    rebuild_renderer(ctx, swapchain);
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

  auto rebuild_renderer(
      const Vulkan::Context& ctx,
      const Vulkan::Swapchain& swapchain) -> void {
    if (!renderer || !renderer->is_valid()) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Cannot build 2D renderer."_view);
    }

    Static::Vector<VkDescriptorSetLayout, 1> descriptor_set_layouts(
        texture.get_descriptor_set_layout());
    const Render::Program* program = renderer->get_program();
    render_program = Vulkan::ShaderProgram::create(
        ctx.get_device(), swapchain.get_format(), *program,
        descriptor_set_layouts);
  }

  auto has_renderer() const -> Bool { return renderer != nullptr; }

  auto render(VkCommandBuffer command_buffer, Bits_32 width, Bits_32 height)
      -> void {
    if (!renderer) {
      return;
    }

    write_host_inputs(width, height);
    render_program.bind(command_buffer);
    render_program.bind_descriptor_set(
        command_buffer, texture.get_descriptor_set());
    if (host_input_size > 0) {
      render_program.push_constants(
          command_buffer, host_input_bytes.get_data(), host_input_size);
    }
    render_program.draw(command_buffer);
  }

 private:
  static auto host_input_size_for(const Render::Program& program) -> Count {
    Count result = 0;
    const auto host_input_ranges = program.get_host_input_ranges();
    for (Count i = 0; i < host_input_ranges.get_size(); i++) {
      const Count range_end = host_input_ranges[i].get_offset() +
                              host_input_ranges[i].get_size();
      if (range_end > result) {
        result = range_end;
      }
    }

    return result;
  }

  template <typename ValueType>
  static auto write_host_field(
      Bits_8* host_input_data,
      Count host_input_size,
      const Render::HostField& field,
      const ValueType* value,
      Count count = 1) -> void {
    const Count byte_count = sizeof(ValueType) * count;
    if (field.get_size() != byte_count ||
        field.get_offset() + field.get_size() > host_input_size) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Invalid 2D renderer host field."_view);
    }

    Data::copy(host_input_data + field.get_offset(), value, count);
  }

  auto set_host_input_size(Count size) -> void {
    if (size > max_host_input_bytes) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: 2D renderer inputs are too large."_view);
    }

    host_input_size = size;
  }

  auto write_host_inputs(Bits_32 width, Bits_32 height) -> void {
    if (!renderer) {
      return;
    }

    const Render::Program* program = renderer->get_program();
    if (!program) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Missing 2D renderer program."_view);
    }

    if (program->get_host_input_ranges().is_empty()) {
      set_host_input_size(0);
      return;
    }
    if (width == 0 || height == 0) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Cannot render to zero-sized extent."_view);
    }

    set_host_input_size(host_input_size_for(*program));
    Data::set(host_input_bytes.get_data(), 0, host_input_size);

    Static::Vector<Real_32, 2> normalized_size_pixels(
        size_pixels[0] / Real_32(width), size_pixels[1] / Real_32(height));

    // The render program describes host-visible fields. Sprite2D owns the
    // engine meaning of those names, so TTX does not need sprite concepts.
    const auto host_fields = program->get_host_fields();
    for (Count i = 0; i < host_fields.get_size(); i++) {
      const Render::HostField& field = host_fields[i];
      const View::Bytes name = NullTerminated::to_view(field.get_name());
      if (name == "position"_view) {
        write_host_field(
            host_input_bytes.get_data(), host_input_size, field,
            position.get_data(), 2);
        continue;
      }
      if (name == "size_pixels"_view) {
        write_host_field(
            host_input_bytes.get_data(), host_input_size, field,
            normalized_size_pixels.get_data(), 2);
        continue;
      }
      if (name == "alpha"_view) {
        write_host_field(
            host_input_bytes.get_data(), host_input_size, field, &alpha);
      }
    }
  }

  Vulkan::Texture texture;
  Vulkan::ShaderProgram render_program;
  const Render* renderer = nullptr;
  Static::Vector<Real_32, 2> position;
  Static::Vector<Real_32, 2> size_pixels;
  Real_32 alpha = 1.0f;
  alignas(16) Static::Bytes<max_host_input_bytes> host_input_bytes;
  Count host_input_size = 0;
};

class WindowState {
 public:
  WindowState(Bits_32 width, Bits_32 height, const char* title)
      : window(width, height, title),
        ctx(Vulkan::Context::create(window)),
        swapchain(
            Vulkan::Swapchain::create(
                ctx,
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
      Diagnostics::Log::fatal("Perimortem::Graphics::Window: Too many 2D sprites."_view);
    }

    const Count result = sprite_2d_count++;
    sprites_2d[result].initialize(ctx, image);
    return result;
  }

  auto set_sprite_2d_renderer(Count sprite_index, const Render& renderer)
      -> void {
    sprites_2d[sprite_index].set_renderer(ctx, swapchain, renderer);
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
    vkWaitForFences(ctx.get_device(), 1, &frame.fence, VK_TRUE, UINT64_MAX);

    const Bits_32 image_index =
        swapchain.acquire_next_image(frame.image_available);
    if (image_index == UINT32_MAX) {
      return;
    }

    vkResetFences(ctx.get_device(), 1, &frame.fence);
    vkResetCommandBuffer(frame.command_buffer, 0);

    record_frame(
        frame.command_buffer, swapchain_images[image_index],
        swapchain.get_image_view(image_index));

    constexpr VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.image_available;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame.command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.render_finished;
    require_success(
        vkQueueSubmit(ctx.get_graphics_queue(), 1, &submit, frame.fence),
        "Perimortem::Graphics::Window: Failed to submit frame."_view);

    const VkSwapchainKHR swapchain_handle = swapchain.get_swapchain();
    VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &frame.render_finished;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_handle;
    present.pImageIndices = &image_index;

    const VkResult present_result =
        vkQueuePresentKHR(ctx.get_graphics_queue(), &present);
    if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR &&
        present_result != VK_ERROR_OUT_OF_DATE_KHR) {
      Diagnostics::Log::fatal(
          "Perimortem::Graphics::Window: Failed to present frame."_view);
    }

    current_frame = (current_frame + 1) % frames_in_flight;
  }

  auto wait_idle() -> void {
    if (ctx.get_device()) {
      vkDeviceWaitIdle(ctx.get_device());
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
    swapchain.recreate(ctx, width, height);
    refresh_swapchain_images();

    if (swapchain.get_format() != old_format) {
      recreate_sprite_2d_renderers();
    }
  }

  auto allocate_frames() -> void {
    VkCommandBufferAllocateInfo command_buffer_allocation = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_allocation.commandPool = ctx.get_command_pool();
    command_buffer_allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocation.commandBufferCount = 1;

    for (Count i = 0; i < frames_in_flight; i++) {
      require_success(
          vkAllocateCommandBuffers(
              ctx.get_device(), &command_buffer_allocation, &frames[i].command_buffer),
          "Perimortem::Graphics::Window: Failed to allocate command buffer."_view);

      VkSemaphoreCreateInfo semaphore_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      require_success(
          vkCreateSemaphore(
              ctx.get_device(), &semaphore_info, nullptr,
              &frames[i].image_available),
          "Perimortem::Graphics::Window: Failed to create image-available semaphore."_view);
      require_success(
          vkCreateSemaphore(
              ctx.get_device(), &semaphore_info, nullptr,
              &frames[i].render_finished),
          "Perimortem::Graphics::Window: Failed to create render-finished semaphore."_view);

      VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      require_success(
          vkCreateFence(
              ctx.get_device(), &fence_info, nullptr, &frames[i].fence),
          "Perimortem::Graphics::Window: Failed to create frame fence."_view);
    }
  }

  auto destroy_frames() -> void {
    for (Count i = 0; i < frames_in_flight; i++) {
      if (frames[i].fence) {
        vkDestroyFence(ctx.get_device(), frames[i].fence, nullptr);
      }
      if (frames[i].render_finished) {
        vkDestroySemaphore(
            ctx.get_device(), frames[i].render_finished, nullptr);
      }
      if (frames[i].image_available) {
        vkDestroySemaphore(
            ctx.get_device(), frames[i].image_available, nullptr);
      }

      frames[i] = {};
    }
  }

  auto refresh_swapchain_images() -> void {
    for (Bits_32 i = 0; i < swapchain.get_image_count(); i++) {
      swapchain_images[i] = swapchain.get_image(i);
    }
  }

  auto recreate_sprite_2d_renderers() -> void {
    for (Count i = 0; i < sprite_2d_count; i++) {
      if (sprites_2d[i].has_renderer()) {
        sprites_2d[i].rebuild_renderer(ctx, swapchain);
      }
    }
  }

  auto record_frame(
      VkCommandBuffer command_buffer,
      VkImage swapchain_image,
      VkImageView swapchain_image_view) -> void {
    VkCommandBufferBeginInfo begin_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require_success(
        vkBeginCommandBuffer(command_buffer, &begin_info),
        "Perimortem::Graphics::Window: Failed to begin command buffer."_view);

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

    VkDependencyInfo attachment_dependency = {
      VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    attachment_dependency.imageMemoryBarrierCount = 1;
    attachment_dependency.pImageMemoryBarriers = &to_attachment;
    vkCmdPipelineBarrier2(command_buffer, &attachment_dependency);

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

    vkCmdBeginRendering(command_buffer, &rendering);

    const VkViewport viewport = {
      0.0f, 0.0f, Real_32(extent.width), Real_32(extent.height), 0.0f, 1.0f};
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const VkRect2D scissor = {{0, 0}, extent};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    for (Count i = 0; i < sprite_2d_count; i++) {
      sprites_2d[i].render(command_buffer, extent.width, extent.height);
    }

    vkCmdEndRendering(command_buffer);

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

    VkDependencyInfo present_dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    present_dependency.imageMemoryBarrierCount = 1;
    present_dependency.pImageMemoryBarriers = &to_present;
    vkCmdPipelineBarrier2(command_buffer, &present_dependency);

    require_success(
        vkEndCommandBuffer(command_buffer),
        "Perimortem::Graphics::Window: Failed to end command buffer."_view);
  }

  Perimortem::System::Window window;
  Vulkan::Context ctx;
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

auto Window::Runtime::run_job(View::Bytes job_data) -> void {
  Reader::Binary<Data::ByteOrder::Native> reader(job_data);
  const Bits_64 runtime_address = reader.read_bits_64();
  if (!reader.is_valid() || !reader.is_empty() || runtime_address == 0) {
    Diagnostics::Log::fatal("Invalid graphics window worker job payload."_view);
  }

  reinterpret_cast<Runtime*>(runtime_address)->run();
}

auto Window::Runtime::run() -> void {
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
    case CommandKind::SetSprite2DRenderer:
      state.set_sprite_2d_renderer(command.sprite_index, *command.renderer);
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

auto Sprite2D::set_renderer(const Render& renderer) -> void {
  window->set_sprite_2d_renderer(index, renderer);
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
  alignas(Bits_64) Static::Bytes<sizeof(Bits_64)> job_data;
  Writer::Binary<Data::ByteOrder::Native> job_writer(job_data.get_access());
  job_writer << reinterpret_cast<Bits_64>(&runtime);
  worker = Core::Thread::Worker::start(
      "graphics.window"_view, Runtime::run_job, job_writer);
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
    Diagnostics::Log::fatal("Perimortem::Graphics::Window: Too many 2D sprites."_view);
  }

  Command command;
  command.kind = CommandKind::CreateSprite2D;
  command.image = &image;
  runtime.execute(command);

  Sprite2D& result = sprites_2d[sprite_2d_count++];
  result.initialize(this, command.sprite_index);
  return result;
}

auto Window::set_sprite_2d_renderer(Count sprite_index, const Render& renderer)
    -> void {
  Command command;
  command.kind = CommandKind::SetSprite2DRenderer;
  command.sprite_index = sprite_index;
  command.renderer = &renderer;
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
        "Perimortem::Graphics::Scene: No active scene for render object creation."_view);
  }
  return *active_scene;
}

auto Scene::create_sprite_2d(const Image& image) -> Sprite2D& {
  return window->create_sprite_2d(image);
}

}  // namespace Perimortem::Graphics
