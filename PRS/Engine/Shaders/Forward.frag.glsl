#version 450

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(set = 0, binding = 0) uniform CameraBlock
{
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
} camera;

// Material base-colour map. Untextured materials bind a 1x1 white image, so the
// shader has a single path.
layout(set = 0, binding = 1) uniform sampler2D albedoMap;

layout(push_constant) uniform ObjectBlock
{
    mat4 model;
    vec4 baseColor;
    vec4 params;
} object;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal = normalize(inNormal);
    vec3 lightDirection = normalize(camera.lightDirection.xyz);
    vec3 viewDirection = normalize(camera.cameraPosition.xyz - inWorldPosition);
    vec3 halfDirection = normalize(lightDirection + viewDirection);

    // Textures are stored as sRGB bytes; decode so lighting happens in linear space.
    vec4 texel = texture(albedoMap, inUV);
    vec3 albedo = inColor.rgb * object.baseColor.rgb * pow(max(texel.rgb, vec3(0.0)), vec3(2.2));

    float diffuseFactor = max(dot(normal, lightDirection), 0.0);
    float shininess = max(object.params.x, 1.0);
    float specularFactor = pow(max(dot(normal, halfDirection), 0.0), shininess) * object.params.y;

    vec3 ambient = albedo * 0.18;
    vec3 diffuse = albedo * diffuseFactor * camera.lightColor.rgb;
    vec3 specular = vec3(specularFactor) * camera.lightColor.rgb;

    vec3 color = ambient + diffuse + specular;
    color = color / (color + vec3(1.0)); // Reinhard tonemap
    outColor = vec4(pow(color, vec3(1.0 / 2.2)), inColor.a * object.baseColor.a * texel.a);
}
