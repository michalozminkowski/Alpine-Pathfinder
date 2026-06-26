#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    gl_Position = projection * viewPos;
    
    // Scale point size based on distance
    float dist = length(viewPos.xyz);
    // Larger size for visibility
    gl_PointSize = max(120.0 / dist, 3.0);
}
