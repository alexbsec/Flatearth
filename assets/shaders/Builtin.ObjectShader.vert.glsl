#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform globalUniformObject {
  mat4 projection;
  mat4 view;
} globalUbo;

void main() {
  gl_Position = vec4(inPosition.xy, 0.5, 1.0);
}
