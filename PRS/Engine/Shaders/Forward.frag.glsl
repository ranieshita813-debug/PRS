#version 450

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(set = 0, binding = 0) uniform CameraBlock
{
    mat4 viewProjection;
    mat4 lightViewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    // x = 1 / shadow map size, y = depth bias, z = normal bias, w = shadows on
    vec4 shadowParams;
} camera;

// Material base-colour map. Untextured materials bind a 1x1 white image, so the
// shader has a single path.
layout(set = 0, binding = 1) uniform sampler2D albedoMap;

// Depth from the sun's point of view. Sampled with a nearest, clamp-to-edge
// sampler; the comparison is done here so no shadow sampler is needed.
layout(set = 0, binding = 2) uniform sampler2D shadowMap;

layout(push_constant) uniform ObjectBlock
{
    mat4 model;
    vec4 baseColor;
    vec4 params;
} object;

layout(location = 0) out vec4 outColor;

// 3x3 percentage-closer filtering: average nine depth comparisons so shadow
// edges soften instead of showing hard stair steps at the map's resolution.
float SampleShadow(vec3 worldPosition, vec3 normal)
{
    if (camera.shadowParams.w < 0.5) return 1.0;

    // Offset along the normal to keep surfaces from shadowing themselves.
    vec4 lightClip =
        camera.lightViewProjection * vec4(worldPosition + normal * camera.shadowParams.z, 1.0);
    vec3 projected = lightClip.xyz / lightClip.w;

    vec2 uv = projected.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;

    float reference = projected.z - camera.shadowParams.y;
    float texel = camera.shadowParams.x;

    float lit = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float stored = texture(shadowMap, uv + vec2(x, y) * texel).r;
            lit += reference <= stored ? 1.0 : 0.0;
        }
    }
    return lit / 9.0;
}

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

    float shadow = SampleShadow(inWorldPosition, normal);

    vec3 ambient = albedo * 0.18;
    vec3 diffuse = albedo * diffuseFactor * camera.lightColor.rgb * shadow;
    vec3 specular = vec3(specularFactor) * camera.lightColor.rgb * shadow;

    vec3 color = ambient + diffuse + specular;
    color = color / (color + vec3(1.0)); // Reinhard tonemap
    outColor = vec4(pow(color, vec3(1.0 / 2.2)), inColor.a * object.baseColor.a * texel.a);
}
