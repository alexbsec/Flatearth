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
  float cornerRadius;
  float quadAspect;
} uPushConstants;

float roundedBoxSDF(vec2 p, vec2 halfExtents, float r) {
  vec2 q = abs(p) - halfExtents + r;
  return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
  vec4 tint = vec4(uPushConstants.rgb, uPushConstants.alpha);
  vec4 texColor = texture(uTexture, vTexCoord);
  outColor = mix(tint, texColor * tint, uPushConstants.useTexture);

  float r = uPushConstants.cornerRadius;
  if (r > 0.0) {
    float qa = uPushConstants.quadAspect;
    vec2 p = (vTexCoord - 0.5) * vec2(qa, 1.0);
    vec2 halfExtents = vec2(qa * 0.5, 0.5);
    float d = roundedBoxSDF(p, halfExtents, r);
    float fw = fwidth(d);
    float mask = 1.0 - smoothstep(-fw, fw, d);
    outColor.a *= mask;
    if (outColor.a <= 0.001) discard;
  }
}
