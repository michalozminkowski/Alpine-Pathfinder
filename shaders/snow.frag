#version 330 core
out vec4 FragColor;

void main() {
    // Soft circular gradient for a misty snowflake look
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0; 
    float dist = length(circCoord);
    
    // Smooth quadratic falloff
    float alpha = pow(max(1.0 - dist, 0.0), 2.0) * 0.6; // 0.6 max opacity for soft look
    
    if (alpha <= 0.01) {
        discard;
    }
    
    FragColor = vec4(1.0, 1.0, 1.0, alpha);
}
