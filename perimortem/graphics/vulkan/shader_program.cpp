// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/vulkan/shader_program.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics::Vulkan;

static constexpr Count max_shader_modules = 8;
static constexpr Count max_push_constant_ranges = 8;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto to_vk_stage(Ttx::ShaderStage stage) -> VkShaderStageFlagBits {
  switch (stage) {
  case Ttx::ShaderStage::Vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case Ttx::ShaderStage::Pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  Diagnostics::Log::fatal("Vulkan: Unknown TTX shader stage."_view);
  return VK_SHADER_STAGE_VERTEX_BIT;
}

static auto to_vk_stage_mask(Bits_32 stage_mask) -> VkShaderStageFlags {
  VkShaderStageFlags flags = 0;
  if ((stage_mask & Ttx::shader_stage_vertex) != 0) {
    flags |= VK_SHADER_STAGE_VERTEX_BIT;
  }
  if ((stage_mask & Ttx::shader_stage_pixel) != 0) {
    flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  if (flags == 0) {
    Diagnostics::Log::fatal("Vulkan: Empty TTX shader stage mask."_view);
  }

  return flags;
}

static auto make_shader_module(VkDevice device, const Ttx::ShaderModule& source)
    -> VkShaderModule {
  if (!source.spirv || !source.spirv_size) {
    Diagnostics::Log::fatal("Vulkan: Invalid TTX shader module."_view);
  }

  VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = *source.spirv_size;
  info.pCode = source.spirv;

  VkShaderModule module = VK_NULL_HANDLE;
  require_success(
      vkCreateShaderModule(device, &info, nullptr, &module),
      "Vulkan: Failed to create shader module."_view);
  return module;
}

auto ShaderProgram::create(
    VkDevice device,
    VkFormat color_format,
    const Ttx::ShaderProgramInfo& shader,
    View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
    -> ShaderProgram {
  if (!shader.modules || shader.module_count == 0 ||
      shader.module_count > max_shader_modules) {
    Diagnostics::Log::fatal("Vulkan: Invalid TTX shader module list."_view);
  }
  if (shader.push_constant_count > max_push_constant_ranges) {
    Diagnostics::Log::fatal(
        "Vulkan: Too many TTX shader push constant ranges."_view);
  }
  if (shader.push_constant_count > 0 && !shader.push_constants) {
    Diagnostics::Log::fatal(
        "Vulkan: Invalid TTX shader push constant metadata."_view);
  }

  ShaderProgram program;
  program.device = device;
  program.source = &shader;

  Static::Vector<VkShaderModule, max_shader_modules> modules;
  Static::Vector<VkPipelineShaderStageCreateInfo, max_shader_modules> stages;
  for (Count i = 0; i < shader.module_count; i++) {
    const Ttx::ShaderModule& module = shader.modules[i];
    modules[i] = make_shader_module(device, module);

    stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[i].stage = to_vk_stage(module.stage);
    stages[i].module = modules[i];
    stages[i].pName = module.entry ? module.entry : "main";
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
  for (Count i = 0; i < shader.push_constant_count; i++) {
    const auto& source_range = shader.push_constants[i];
    push_ranges[i].stageFlags = to_vk_stage_mask(source_range.stage_mask);
    push_ranges[i].offset = Bits_32(source_range.offset);
    push_ranges[i].size = Bits_32(source_range.size);
  }

  VkPipelineLayoutCreateInfo layout_info = {
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.setLayoutCount = Bits_32(descriptor_set_layouts.get_size());
  layout_info.pSetLayouts = descriptor_set_layouts.get_data();
  layout_info.pushConstantRangeCount = Bits_32(shader.push_constant_count);
  layout_info.pPushConstantRanges = push_ranges.get_data();

  require_success(
      vkCreatePipelineLayout(device, &layout_info, nullptr, &program.layout),
      "Vulkan: Failed to create shader pipeline layout."_view);

  VkPipelineRenderingCreateInfo rendering = {
    VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachmentFormats = &color_format;

  VkGraphicsPipelineCreateInfo pipeline_info = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipeline_info.pNext = &rendering;
  pipeline_info.stageCount = Bits_32(shader.module_count);
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
      "Vulkan: Failed to create shader pipeline."_view);

  for (Count i = 0; i < shader.module_count; i++) {
    vkDestroyShaderModule(device, modules[i], nullptr);
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
      source(other.source) {
  other.device = VK_NULL_HANDLE;
  other.layout = VK_NULL_HANDLE;
  other.pipeline = VK_NULL_HANDLE;
  other.source = nullptr;
}

auto ShaderProgram::operator=(ShaderProgram&& other) noexcept
    -> ShaderProgram& {
  if (this != &other) {
    this->~ShaderProgram();
    device = other.device;
    layout = other.layout;
    pipeline = other.pipeline;
    source = other.source;
    other.device = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
    other.pipeline = VK_NULL_HANDLE;
    other.source = nullptr;
  }
  return *this;
}

auto ShaderProgram::bind(VkCommandBuffer cmd) const -> void {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

auto ShaderProgram::bind_descriptor_set(
    VkCommandBuffer cmd,
    VkDescriptorSet descriptor_set,
    Count descriptor_index) const -> void {
  if (!source || descriptor_index >= source->descriptor_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid TTX shader descriptor."_view);
  }

  const auto& descriptor = source->descriptors[descriptor_index];
  vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Bits_32(descriptor.set), 1,
      &descriptor_set, 0, nullptr);
}

auto ShaderProgram::push_constants(
    VkCommandBuffer cmd,
    const void* data,
    Count size,
    Count range_index) const -> void {
  if (!source || range_index >= source->push_constant_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid TTX shader push constants."_view);
  }

  const auto& range = source->push_constants[range_index];
  if (size > range.size) {
    Diagnostics::Log::fatal(
        "Vulkan: TTX shader push constants are too large."_view);
  }

  vkCmdPushConstants(
      cmd, layout, to_vk_stage_mask(range.stage_mask), Bits_32(range.offset),
      Bits_32(size), data);
}

auto ShaderProgram::draw(VkCommandBuffer cmd) const -> void {
  if (!source) {
    Diagnostics::Log::fatal(
        "Vulkan: Missing TTX shader program metadata."_view);
  }

  vkCmdDraw(cmd, Bits_32(source->vertex_count), 1, 0, 0);
}

auto ShaderProgram::get_layout() const -> VkPipelineLayout {
  return layout;
}

auto ShaderProgram::get_pipeline() const -> VkPipeline {
  return pipeline;
}
