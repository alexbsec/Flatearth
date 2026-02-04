#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vNdc;     
layout(location = 0) out vec4 outColor;

void main() {
  float t = clamp(vNdc.x * 0.5 + 0.5, vNdc.y + 0.5, 1.0);

  vec3 darkRed   = vec3(0.2, 0.0, 0.0);
  vec3 brightRed = vec3(1.0, vNdc.y, 0.0);

  outColor = vec4(mix(darkRed, brightRed, t), 1.0);
}
