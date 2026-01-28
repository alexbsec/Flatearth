#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 out_colour;

void main() {
  out_colour = vec4(inPosition.r + 0.5, inPosition.g + 0.5, inPosition.b + 0.5, 1.0);
}
