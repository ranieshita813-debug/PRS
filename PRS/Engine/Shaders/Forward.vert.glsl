#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform CameraBlock
{
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
} camera;

layout(push_constant) uniform ObjectBlock
{
    mat4 model;
    vec4 baseColor;
    vec4 params;
} object;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;

void main()
{
    vec4 world = object.model * vec4(inPosition, 1.0);
    outWorldPosition = world.xyz;
    outNormal = normalize(mat3(object.model) * inNormal);
    outUV = inUV;
    outColor = object.baseColor;
    gl_Position = camera.viewProjection * world;
}
