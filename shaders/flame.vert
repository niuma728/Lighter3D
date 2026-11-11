#version 330 core
layout(location = 0) in vec4 position;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 anchor;
uniform float time;
uniform float scale;
out float height;
out float layer;
void main() {
    height = position.y;
    layer = position.w;
    float h = height;
    float innerHeight = layer > 0.5 ? 0.68 : 1.0;
    vec3 p = vec3(position.x, h * 0.82 * innerHeight, position.z);
    p.x += 0.06 * h * h * innerHeight * sin(time * 7.0 + 5.0 * h);
    p.z += 0.035 * h * h * innerHeight * sin(time * 9.0 + 6.0 * h + 1.0);
    p.y *= 1.0 + 0.035 * sin(time * 11.0) + 0.02 * sin(time * 17.0);
    gl_Position = projection * view * vec4(anchor + p * scale, 1.0);
}
