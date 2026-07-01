// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/shader_program.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/static/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics::Vulkan;

static constexpr Count max_shader_modules = 8;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto to_vk_stage(Perimortem::Graphics::Render::Stage stage)
    -> VkShaderStageFlagBits {
  switch (stage) {
  case Perimortem::Graphics::Render::Stage::Vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case Perimortem::Graphics::Render::Stage::Pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  Diagnostics::Log::fatal("Vulkan: Unknown render stage."_view);
  return VK_SHADER_STAGE_VERTEX_BIT;
}

static auto to_vk_stage_flags(
    View::Vector<Perimortem::Graphics::Render::Stage> stages)
    -> VkShaderStageFlags {
  VkShaderStageFlags flags = 0;
  for (Count i = 0; i < stages.get_size(); i++) {
    flags |= to_vk_stage(stages[i]);
  }

  if (flags == 0) {
    Diagnostics::Log::fatal("Vulkan: Empty render stage list."_view);
  }

  return flags;
}

static auto make_shader_module(
    VkDevice device,
    const Perimortem::Graphics::Render::Module& source) -> VkShaderModule {
  if (!source.get_spirv() || !source.get_spirv_size()) {
    Diagnostics::Log::fatal("Vulkan: Invalid render module."_view);
  }

  VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = *source.get_spirv_size();
  info.pCode = source.get_spirv();

  VkShaderModule module = VK_NULL_HANDLE;
  require_success(
      vkCreateShaderModule(device, &info, nullptr, &module),
      "Vulkan: Failed to create render module."_view);
  return module;
}

auto ShaderProgram::create(
    VkDevice device,
    VkFormat color_format,
    const Perimortem::Graphics::Render::Program& render,
    View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
    -> ShaderProgram {
  const auto source_modules = render.get_modules();
  const auto source_host_input_ranges = render.get_host_input_ranges();
  const auto source_descriptors = render.get_descriptors();

  if (source_modules.is_empty() ||
      source_modules.get_size() > max_shader_modules) {
    Diagnostics::Log::fatal("Vulkan: Invalid render module list."_view);
  }
  if (source_host_input_ranges.get_size() > max_push_constant_ranges) {
    Diagnostics::Log::fatal(
        "Vulkan: Too many render host input ranges."_view);
  }
  if (source_descriptors.get_size() > max_descriptor_bindings) {
    Diagnostics::Log::fatal(
        "Vulkan: Too many render descriptor bindings."_view);
  }

  ShaderProgram program;
  program.device = device;
  program.vertex_count = render.get_vertex_count();
  program.push_constant_count = source_host_input_ranges.get_size();
  program.descriptor_count = source_descriptors.get_size();

  for (Count i = 0; i < program.push_constant_count; i++) {
    const auto& source_range = source_host_input_ranges[i];
    program.push_constant_ranges[i] = ShaderProgram::PushConstantRange(
        to_vk_stage_flags(source_range.get_stages()),
        source_range.get_offset(), source_range.get_size());
  }

  for (Count i = 0; i < program.descriptor_count; i++) {
    const auto& source_descriptor = source_descriptors[i];
    program.descriptors[i] = ShaderProgram::DescriptorBinding(
        source_descriptor.get_set(), source_descriptor.get_slot());
  }

  Static::Vector<VkShaderModule, max_shader_modules> shader_modules;
  Static::Vector<VkPipelineShaderStageCreateInfo, max_shader_modules> stages;
  for (Count i = 0; i < source_modules.get_size(); i++) {
    const auto& module = source_modules[i];
    shader_modules[i] = make_shader_module(device, module);

    stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[i].stage = to_vk_stage(module.get_stage());
    stages[i].module = shader_modules[i];
    stages[i].pName = module.get_entry() ? module.get_entry() : "main";
  }

  VkPipelineVertexInputStateCreateInfo vertex_input = {
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewport_state = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState blend_attachment = {};
  blend_attachment.blendEnable = VK_TRUE;
  blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
  blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo color_blending = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &blend_attachment;

  Static::Vector<VkDynamicState, 2> dynamic_states(
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR);
  VkPipelineDynamicStateCreateInfo dynamic_state = {
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamic_state.dynamicStateCount = Bits_32(dynamic_states.get_size());
  dynamic_state.pDynamicStates = dynamic_states.get_data();

  Static::Vector<VkPushConstantRange, max_push_constant_ranges> push_ranges;
  for (Count i = 0; i < program.push_constant_count; i++) {
    const auto& source_range = program.push_constant_ranges[i];
    push_ranges[i].stageFlags = source_range.get_stage_flags();
    push_ranges[i].offset = Bits_32(source_range.get_offset());
    push_ranges[i].size = Bits_32(source_range.get_size());
  }

  VkPipelineLayoutCreateInfo layout_info = {
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.setLayoutCount = Bits_32(descriptor_set_layouts.get_size());
  layout_info.pSetLayouts = descriptor_set_layouts.get_data();
  layout_info.pushConstantRangeCount = Bits_32(program.push_constant_count);
  layout_info.pPushConstantRanges = push_ranges.get_data();

  require_success(
      vkCreatePipelineLayout(device, &layout_info, nullptr, &program.layout),
      "Vulkan: Failed to create render pipeline layout."_view);

  VkPipelineRenderingCreateInfo rendering = {
    VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachmentFormats = &color_format;

  VkGraphicsPipelineCreateInfo pipeline_info = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipeline_info.pNext = &rendering;
  pipeline_info.stageCount = Bits_32(source_modules.get_size());
  pipeline_info.pStages = stages.get_data();
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = program.layout;

  require_success(
      vkCreateGraphicsPipelines(
          device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
          &program.pipeline),
      "Vulkan: Failed to create render pipeline."_view);

  for (Count i = 0; i < source_modules.get_size(); i++) {
    vkDestroyShaderModule(device, shader_modules[i], nullptr);
  }

  return program;
}

ShaderProgram::~ShaderProgram() {
  if (!device) {
    return;
  }
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, layout, nullptr);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : device(other.device),
      layout(other.layout),
      pipeline(other.pipeline),
      push_constant_ranges(other.push_constant_ranges),
      descriptors(other.descriptors),
      push_constant_count(other.push_constant_count),
      descriptor_count(other.descriptor_count),
      vertex_count(other.vertex_count) {
  other.device = VK_NULL_HANDLE;
  other.layout = VK_NULL_HANDLE;
  other.pipeline = VK_NULL_HANDLE;
  other.push_constant_count = 0;
  other.descriptor_count = 0;
  other.vertex_count = 0;
}

auto ShaderProgram::operator=(ShaderProgram&& other) noexcept
    -> ShaderProgram& {
  if (this != &other) {
    this->~ShaderProgram();
    device = other.device;
    layout = other.layout;
    pipeline = other.pipeline;
    push_constant_ranges = other.push_constant_ranges;
    descriptors = other.descriptors;
    push_constant_count = other.push_constant_count;
    descriptor_count = other.descriptor_count;
    vertex_count = other.vertex_count;
    other.device = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
    other.pipeline = VK_NULL_HANDLE;
    other.push_constant_count = 0;
    other.descriptor_count = 0;
    other.vertex_count = 0;
  }
  return *this;
}

auto ShaderProgram::bind(VkCommandBuffer command_buffer) const -> void {
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

auto ShaderProgram::bind_descriptor_set(
    VkCommandBuffer command_buffer,
    VkDescriptorSet descriptor_set,
    Count descriptor_index) const -> void {
  if (descriptor_index >= descriptor_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid render descriptor."_view);
  }

  const auto& descriptor = descriptors[descriptor_index];
  vkCmdBindDescriptorSets(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
      Bits_32(descriptor.get_set()), 1, &descriptor_set, 0, nullptr);
}

auto ShaderProgram::push_constants(
    VkCommandBuffer command_buffer,
    const void* constant_data,
    Count size,
    Count range_index) const -> void {
  if (range_index >= push_constant_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid render host inputs."_view);
  }

  const auto& range = push_constant_ranges[range_index];
  if (size > range.get_size()) {
    Diagnostics::Log::fatal(
        "Vulkan: Render host inputs are too large."_view);
  }

  vkCmdPushConstants(
      command_buffer, layout, range.get_stage_flags(),
      Bits_32(range.get_offset()), Bits_32(size), constant_data);
}

auto ShaderProgram::draw(VkCommandBuffer command_buffer) const -> void {
  if (vertex_count == 0) {
    Diagnostics::Log::fatal("Vulkan: Missing render draw metadata."_view);
  }

  vkCmdDraw(command_buffer, Bits_32(vertex_count), 1, 0, 0);
}

auto ShaderProgram::get_layout() const -> VkPipelineLayout {
  return layout;
}

auto ShaderProgram::get_pipeline() const -> VkPipeline {
  return pipeline;
}
