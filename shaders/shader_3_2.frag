#version 330 core

uniform vec3 objectColor;
uniform sampler2D textureSampler;
uniform sampler2D normalSampler;
uniform float snowLevel; // 0.0 to 1.0
in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

out vec4 out_color;

// Pseudo-random hash function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// 2D Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    // Four corners
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Fractal Brownian Motion for more natural, detailed noise
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Derivative based Normal Mapping (No precomputed tangents needed)
vec3 perturbNormal(vec3 baseNormal, vec3 p, vec2 uv) {
    vec3 tangentNormal = texture(normalSampler, uv).xyz * 2.0 - 1.0;
    
    // We compute the TBN matrix on the fly using screen-space derivatives
    vec3 q0 = dFdx(p);
    vec3 q1 = dFdy(p);
    vec2 st0 = dFdx(uv);
    vec2 st1 = dFdy(uv);

    vec3 N = normalize(baseNormal);
    vec3 q1perp = cross(q1, N);
    vec3 q0perp = cross(N, q0);

    vec3 T = q1perp * st0.x + q0perp * st1.x;
    vec3 B = q1perp * st0.y + q0perp * st1.y;

    float det = max(dot(T, T), dot(B, B));
    float scale = (det == 0.0) ? 0.0 : inversesqrt(det);

    return normalize((T * tangentNormal.x + B * tangentNormal.y) * scale + N * tangentNormal.z);
}

void main()
{
    // Apply Normal Mapping
    vec3 N = perturbNormal(normal, fragPos, texCoord);
    
    // Simple directional light from top-right-front
    vec3 L = normalize(vec3(0.5, 1.0, 0.5));
    float diffuse = max(dot(N, L), 0.0);
    
    // Ambient light (brightened to make texture details more visible)
    vec3 ambient = vec3(0.4, 0.4, 0.4);
    
    // Sample the base texture of the desert
    vec3 texColor = texture(textureSampler, texCoord).rgb;
    
    // Combine diffuse and ambient directly with the pure texture color
    // (Removed the procedural dirt noise here because the new texture already has detailed dirt/sand)
    vec3 color = texColor * (diffuse * 0.8 + 0.2) + ambient * 0.2;
    
    // Snow calculation
    // As snowLevel increases from 0.0 to 5.0, snow can stick to steeper and steeper slopes
    // N.y represents the upward normal (1.0 = flat ground, 0.0 = vertical wall)
    float minSlope = max(0.8 - (snowLevel * 0.15), 0.1); 
    float maxSlope = max(0.95 - (snowLevel * 0.1), 0.2); 
    
    // Activate the noise function! We use FBM noise to perturb the slope calculation.
    // This breaks up the perfectly straight snow lines and makes them look organic.
    float noiseVal = fbm(texCoord * 150.0); 
    float perturbedNy = N.y + (noiseVal - 0.5) * 0.2; // Add random jitter to the surface angle
    
    float slopeFactor = smoothstep(minSlope, maxSlope, perturbedNy);
    float snowCoverage = clamp(slopeFactor * snowLevel, 0.0, 1.0);
    
    // We also use a bit of high-frequency noise to make the snow sparkle slightly, 
    // avoiding a perfectly flat plastic look.
    float snowSparkle = fbm(texCoord * 500.0) * 0.1;
    vec3 snowColor = vec3(0.9, 0.95, 1.0) * (diffuse * 0.85 + 0.15 + snowSparkle) + ambient * 0.2;
    
    // Mix the original rock color with the snow color
    color = mix(color, snowColor, snowCoverage);
    
    out_color = vec4(color, 1.0);
}
