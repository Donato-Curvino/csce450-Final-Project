#version 120

attribute vec4 point;
// attribute vec3 color;

uniform mat4 P;
uniform mat4 MV;

varying vec4 vColor;

void main() {
    gl_Position = P * MV * vec4(point.xyz, 1.f);
    gl_PointSize = (point.w == 0) ? 10 : 20;
    vColor = (point.w == 0)
        ? vec4(1, 0, 0, 1)
        : vec4(1, 1, 0, 1);
}
