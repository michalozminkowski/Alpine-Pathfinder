#version 330 core

uniform vec3 objectColor;
uniform sampler2D textureSampler;
uniform float snowLevel; // 0.0 to 1.0
in vec3 normal;
in vec2 texCoord;

out vec4 out_color;

void main()
{
    vec3 N = normalize(normal);
    // Simple directional light from top-right-front
    vec3 L = normalize(vec3(0.5, 1.0, 0.5));
    float diffuse = max(dot(N, L), 0.0);
    
    // Ambient light simulating sky blue glow
    vec3 ambient = vec3(0.15, 0.2, 0.3);
    
    // Sample texture
    vec3 texColor = texture(textureSampler, texCoord).rgb;
    
    // Combine diffuse and ambient
    vec3 color = texColor * (diffuse * 0.85 + 0.15) + ambient * 0.1;
    
    // Snow calculation
    // As snowLevel increases from 0.0 to 5.0, snow can stick to steeper and steeper slopes
    // N.y represents the upward normal (1.0 = flat ground, 0.0 = vertical wall)
    float minSlope = max(0.8 - (snowLevel * 0.15), 0.1); 
    float maxSlope = max(0.95 - (snowLevel * 0.1), 0.2); 
    
    float slopeFactor = smoothstep(minSlope, maxSlope, N.y);
    float snowCoverage = clamp(slopeFactor * snowLevel, 0.0, 1.0);
    
    vec3 snowColor = vec3(0.9, 0.95, 1.0) * (diffuse * 0.85 + 0.15) + ambient * 0.2;
    
    // Mix the original rock color with the snow color
    color = mix(color, snowColor, snowCoverage);
    
    out_color = vec4(color, 1.0);
}
