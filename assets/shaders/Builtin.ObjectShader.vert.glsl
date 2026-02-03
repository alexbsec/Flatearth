#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform globalUniformObject {
  mat4 projection;
  mat4 view;
} globalUbo;

layout(push_constant) uniform pushConstants {
  mat4 model;
} uPushConstants;

void main() {
  gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
