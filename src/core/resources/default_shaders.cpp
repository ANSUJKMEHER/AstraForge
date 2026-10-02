#include "af/resources/Shaders.h"

namespace af {

// GLSL 330 core vertex shader.
const char* DefaultVertexShader() {
    return R"GLSL(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec3 vWorldPosition;
out vec2 vUV;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPosition = world.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vUV = aUV;
    gl_Position = uViewProjection * world;
}
)GLSL";
}

// Modern stylized lighting with crisp emissive grid, ground AO, and alpha blending.
const char* DefaultFragmentShader() {
    return R"GLSL(#version 330 core
in vec3 vNormal;
in vec3 vWorldPosition;
in vec2 vUV;

uniform vec3 uColor;
uniform float uAlpha;
uniform vec3 uLightDirection;  // direction light travels
uniform vec3 uLightColor;
uniform vec3 uCameraPosition;
uniform float uAmbientStrength;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDirection);
    vec3 viewDir = normalize(uCameraPosition - vWorldPosition);

    // 1. Soft Half-Lambert Diffuse Lighting (Zelda/Pixar curve)
    float NdotL = max(dot(n, lightDir), 0.0);
    float diffuse = smoothstep(0.0, 0.70, NdotL) * 0.85 + 0.15 * NdotL;

    // 2. Hemispheric Sky/Ground Ambient Lighting
    vec3 skyAmbient = vec3(0.35, 0.42, 0.55) * (uAmbientStrength * 1.5);
    vec3 groundAmbient = vec3(0.18, 0.20, 0.28) * uAmbientStrength;
    vec3 ambient = mix(groundAmbient, skyAmbient, n.y * 0.5 + 0.5);

    // 3. Specular Highlights
    vec3 halfDir = normalize(viewDir + lightDir);
    float specTerm = max(dot(n, halfDir), 0.0);
    float specular = pow(specTerm, 32.0) * 0.50;

    // 4. Fresnel Rim Lighting for 3D Silhouettes
    float NdotV = max(dot(n, viewDir), 0.0);
    float fresnel = pow(1.0 - NdotV, 3.0);
    vec3 rim = uLightColor * (fresnel * 0.45);

    // 5. Stylized Base Material & Floor Shading
    vec3 baseColor = uColor;
    vec3 emissiveGlow = vec3(0.0);
    bool isFloor = (n.y > 0.7 && abs(vWorldPosition.y) < 0.2);

    if (isFloor) {
        vec2 coord = vWorldPosition.xz;

        if (uColor.y > uColor.x * 1.3 && uColor.y > uColor.z * 1.3) {
            // Checkered Grass Lawn Pattern (PvZ Garden Theme)
            vec2 tileCoord = floor(coord * 0.5);
            float check = mod(tileCoord.x + tileCoord.y, 2.0);
            vec3 grassLight = vec3(0.30, 0.75, 0.30);
            vec3 grassDark  = vec3(0.20, 0.60, 0.22);
            baseColor = mix(grassDark, grassLight, check);

            // Subtle border grid lines
            vec2 grid = abs(fract(coord * 0.5 - 0.5) - 0.5) / fwidth(coord * 0.5);
            float gridLine = 1.0 - min(min(grid.x, grid.y), 1.0);
            baseColor = mix(baseColor, vec3(0.14, 0.45, 0.16), gridLine * 0.5);
        } else {
            // High-Tech Cyber Grid Floor with Glowing Neon Lines
            vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
            float line = 1.0 - min(min(grid.x, grid.y), 1.0);
            vec2 grid5 = abs(fract(coord * 0.2 - 0.5) - 0.5) / fwidth(coord * 0.2);
            float line5 = 1.0 - min(min(grid5.x, grid5.y), 1.0);

            vec3 tileDark = vec3(0.10, 0.12, 0.18);
            vec3 subGridColor = vec3(0.20, 0.40, 0.60);
            vec3 neonGridColor = vec3(0.20, 0.85, 1.0);  // Brilliant Electric Cyan

            baseColor = tileDark;
            emissiveGlow += subGridColor * (line * 0.5);
            emissiveGlow += neonGridColor * (line5 * 0.95);  // Radiates luminous neon
        }

        // Soft perimeter vignette
        float arenaDist = length(coord) / 24.0;
        float arenaVignette = smoothstep(1.0, 0.6, arenaDist);
        baseColor *= (0.55 + 0.45 * arenaVignette);
    } else {
        // Vertical Ambient Occlusion (darkens slightly near the ground)
        float groundAO = clamp((vWorldPosition.y + 0.1) * 0.5 + 0.5, 0.55, 1.0);
        baseColor *= groundAO;

        // Extra emissive punch for lasers and glowing items
        float brightness = dot(uColor, vec3(0.299, 0.587, 0.114));
        if (brightness > 0.85) {
            emissiveGlow += uColor * 0.8;
        }
    }

    // 6. Composition
    vec3 lit = baseColor * (ambient + diffuse * uLightColor);
    vec3 specHighlight = uLightColor * specular;
    vec3 finalColor = lit + specHighlight + rim + emissiveGlow;

    // 7. Distance Fog (Starts well outside the 20m arena radius)
    float camDist = length(uCameraPosition - vWorldPosition);
    float fogFactor = clamp((camDist - 36.0) / 32.0, 0.0, 0.85);
    vec3 horizonColor = vec3(0.08, 0.11, 0.18);
    finalColor = mix(finalColor, horizonColor, fogFactor);

    float alpha = (uAlpha > 0.001) ? uAlpha : 1.0;
    fragColor = vec4(finalColor, alpha);
}
)GLSL";
}

}  // namespace af
