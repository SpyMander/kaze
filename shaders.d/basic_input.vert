#version 450

// in here, center i 0
// y is 1 at bottom 

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV;
layout(location=0) out vec3 fragColor;

layout (set = 0, binding = 0) uniform Buf {
  float colorOffset;
} buf;


void main() {
     gl_Position = vec4(inPosition + buf.colorOffset, 1.0);
     fragColor = vec3(inUV, 0.0);
}
