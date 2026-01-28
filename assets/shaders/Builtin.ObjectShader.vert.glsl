#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outPosition;

void main() {
  gl_Position = vec4(inPosition, 1.0);
  outPosition = inPosition;
}
