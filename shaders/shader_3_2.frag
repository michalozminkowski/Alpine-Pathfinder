#version 330 core

uniform vec3 objectColor;
uniform sampler2D textureSampler;
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
    
    out_color = vec4(color, 1.0);
}
