// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#version 450

layout(location = 0) in vec2 texture_uv;
layout(location = 0) out vec4 output_color;
layout(set = 0, binding = 0) uniform sampler2D image;

layout(push_constant) uniform SpriteInputs {
  vec4 transform_x;
  vec4 transform_y;
  vec4 tone;
} sprite;

void main() {
  output_color = texture(image, texture_uv) * sprite.tone;
}
