// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#version 450

layout(location = 0) out vec2 texture_uv;

layout(push_constant) uniform SpriteInputs {
  vec4 transform_x;
  vec4 transform_y;
  vec4 tone;
} sprite;

const vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0));

void main() {
  vec2 local = positions[gl_VertexIndex];
  vec2 pixel = vec2(
      dot(sprite.transform_x.xy, local) + sprite.transform_x.z,
      dot(sprite.transform_y.xy, local) + sprite.transform_y.z);
  vec2 clip = vec2(
      pixel.x / sprite.transform_x.w * 2.0 - 1.0,
      1.0 - pixel.y / sprite.transform_y.w * 2.0);
  gl_Position = vec4(clip, 0.0, 1.0);
  texture_uv = local;
}
