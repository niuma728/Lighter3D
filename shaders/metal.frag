#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 eye;
uniform vec3 baseColor;
uniform float shininess;
uniform float specularStrength;
uniform vec3 flamePosition;
uniform float flamePower;
out vec4 color;

vec3 illuminate(vec3 light, vec3 tint, vec3 n, vec3 v) {
    float diffuse = max(dot(n, light), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0) specular = pow(max(dot(n, normalize(light + v)), 0.0), shininess);
    return tint * (baseColor * diffuse * 0.65 + vec3(specularStrength) * specular);
}

void main() {
    vec3 n = normalize(worldNormal);
    if (!gl_FrontFacing) n = -n;
    vec3 v = normalize(eye - worldPosition);
    vec3 result = baseColor * 0.20;
    result += illuminate(normalize(vec3(-3.0, 5.0, 4.0)), vec3(1.0, 0.95, 0.88), n, v);
    vec3 delta = vec3(3.0, 2.5, -3.0) - worldPosition;
    result += illuminate(normalize(delta), vec3(0.65, 0.78, 1.0), n, v) / (1.0 + 0.025 * dot(delta, delta));
    vec3 warm = flamePosition + vec3(0.0, 0.15, 0.0) - worldPosition;
    result += flamePower * illuminate(normalize(warm), vec3(1.0, 0.36, 0.06), n, v) / (1.0 + 12.0 * dot(warm, warm));
    color = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
