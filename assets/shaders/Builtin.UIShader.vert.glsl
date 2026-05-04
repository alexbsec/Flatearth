#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform globalUniformObject {
  layout(row_major) mat4 projection;
  layout(row_major) mat4 view;
} globalUbo;

layout(push_constant) uniform pushConstants {
  layout(row_major) mat4 model;
  vec2 uvOffset;
  vec2 uvScale;
  vec3 rgb;
  float alpha;
  float useTexture;
  float cornerRadius;
  float quadAspect;
} uPushConstants;

layout(location = 0) out vec2 vTexCoord;

void main() {
  gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model *
    vec4(inPosition, 1.0);
  vTexCoord = uPushConstants.uvOffset + inTexCoord * uPushConstants.uvScale;
}
