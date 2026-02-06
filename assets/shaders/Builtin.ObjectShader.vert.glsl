#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform globalUniformObject {
  layout(row_major) mat4 projection;
  layout(row_major) mat4 view;
} globalUbo;

layout(push_constant) uniform pushConstants {
  layout(row_major) mat4 model;
} uPushConstants;

layout(location = 0) out vec2 vNdc; 

void main() {
  vec4 clip = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
  gl_Position = clip;

  vNdc = clip.xy / clip.w;
}
