#version 330 core

uniform vec3 objectColor;
uniform sampler2D textureSampler;
uniform sampler2D normalSampler;
uniform float snowLevel; // 0.0 to 1.0
uniform float snowNoiseScale;
uniform float snowDistortion;
in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

out vec4 out_color;

// Pseudolosowa funkcja szumu
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // interpolacja
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Fractal Brownian Motion dla bardziej naturalnego, szczegółowego szumu
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

// Normal Mapping oparte na pochodnych
vec3 perturbNormal(vec3 baseNormal, vec3 p, vec2 uv) {
    vec3 tangentNormal = texture(normalSampler, uv).xyz * 2.0 - 1.0;
    
    // Obliczamy macierz TBN w locie za pomocą pochodnych przestrzeni ekranu
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
    
    // Zabezpieczenie przed ekstremalnym rozciągnięciem UV na klifach
    if (det < 1e-7) {
        return normalize(baseNormal);
    }
    
    float scale = inversesqrt(det);
    // Ograniczamy maksymalne skalowanie, co sprawia, że na bardzo rozciągniętych 
    // trójkątach efekt normal mapy płynnie zanika na rzecz bazowego kształtu
    scale = min(scale, 10.0);

    return normalize((T * tangentNormal.x + B * tangentNormal.y) * scale + N * tangentNormal.z);
}

void main()
{
    // Zastosowanie mapowania normalnych
    vec3 N = perturbNormal(normal, fragPos, texCoord);
    
    // Światło kierunkowe z góry-prawej-przodu
    vec3 L = normalize(vec3(0.5, 1.0, 0.5));
    float diffuse = max(dot(N, L), 0.0);
    
    // Światło otoczenia (ambient)
    vec3 ambient = vec3(0.4, 0.4, 0.4);
    
    // Próbkowanie bazowej tekstury modelu
    vec3 texColor = texture(textureSampler, texCoord).rgb;
    
    // Połączenie diffuse i ambient z kolorem tekstury
    vec3 color = texColor * (diffuse * 0.8 + 0.2) + ambient * 0.2;
    
    // Obliczanie pokrywy śnieżnej
    float minSlope = max(0.8 - (0.003 * snowLevel * snowLevel), 0.5); 
    float maxSlope = max(0.95 - (0.0015 * snowLevel * snowLevel), 0.8); 
    
    // Funkcja szumu
    float noiseVal = fbm(texCoord * snowNoiseScale); 
    float perturbedNy = N.y + (noiseVal - 0.5) * snowDistortion;
    
    float slopeFactor = smoothstep(minSlope, maxSlope, perturbedNy);
    float snowCoverage = clamp(slopeFactor * snowLevel, 0.0, 1.0);
    
    // Szum o wysokiej częstotliwości, aby śnieg lekko błyszczał.
    float snowSparkle = fbm(texCoord * 500.0) * 0.1;
    vec3 snowColor = vec3(0.9, 0.95, 1.0) * (diffuse * 0.85 + 0.15 + snowSparkle) + ambient * 0.2;
    
    // Mieszanie koloru skały z kolorem śniegu
    color = mix(color, snowColor, snowCoverage);
    
    out_color = vec4(color, 1.0);
}
