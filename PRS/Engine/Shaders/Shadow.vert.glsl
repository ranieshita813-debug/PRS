#version 450

// Depth-only pass: the sun's orthographic camera writes just the closest
// fragments, which the forward pass then compares against.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform ShadowBlock
{
    mat4 lightViewProjection;
    mat4 model;
} shadow;

void main()
{
    gl_Position = shadow.lightViewProjection * shadow.model * vec4(inPosition, 1.0);
}
