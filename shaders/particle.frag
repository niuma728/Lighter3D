#version 330 core
in float fade;
in float kind;
out vec4 color;
void main() {
    float r = length(gl_PointCoord * 2.0 - 1.0);
    if (r > 1.0) discard;
    float alpha = (1.0 - smoothstep(0.0, 1.0, r)) * fade;
    if (kind > 0.5) color = vec4(0.50, 0.53, 0.58, alpha * 0.16);
    else color = vec4(mix(vec3(1.0, 0.22, 0.02), vec3(1.0, 0.93, 0.55), fade), alpha);
}
