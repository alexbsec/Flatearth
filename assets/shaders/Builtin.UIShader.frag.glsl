#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform pushConstants {
  layout(row_major) mat4 model;
  vec2 uvOffset;
  vec2 uvScale;
  vec3 rgb;
  float alpha;
  float useTexture;
} uPushConstants;

void main() {
  vec4 tint = vec4(uPushConstants.rgb, uPushConstants.alpha);
  vec4 texColor = texture(uTexture, vTexCoord);
  outColor = mix(tint, texColor * tint, uPushConstants.useTexture);
}
