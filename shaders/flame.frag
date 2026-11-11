#version 330 core
in float height;
in float layer;
uniform float scale;
out vec4 color;
void main() {
    float h = clamp(height, 0.0, 1.0);
    vec3 outer = mix(vec3(1.0, 0.82, 0.13), vec3(1.0, 0.18, 0.015), smoothstep(0.25, 1.0, h));
    vec3 inner = mix(vec3(1.0, 0.98, 0.75), vec3(1.0, 0.68, 0.12), smoothstep(0.25, 1.0, h));
    vec3 tint = mix(outer, inner, layer);
    tint = mix(vec3(0.16, 0.38, 1.0), tint, smoothstep(0.02, 0.20, h));
    float alpha = mix(0.32, 0.70, layer) * pow(1.0 - h, 0.28) * scale;
    alpha *= smoothstep(0.0, 0.04, h);
    color = vec4(tint * (0.65 + 0.35 * scale), alpha);
}
