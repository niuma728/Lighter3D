#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in float life;
layout(location = 2) in float smoke;
uniform mat4 view;
uniform mat4 projection;
uniform float viewportHeight;
out float fade;
out float kind;
void main() {
    gl_Position = projection * view * vec4(position, 1.0);
    fade = life;
    kind = smoke;
    float size = smoke > 0.5 ? mix(0.20, 0.05, life) : mix(0.012, 0.035, life);
    gl_PointSize = clamp(size * viewportHeight / gl_Position.w, 1.0, 80.0);
}
