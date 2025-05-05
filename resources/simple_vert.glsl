#version 120

attribute vec3 vertex;

uniform mat4 P;
uniform mat4 MV;

varying vec4 vColor;

void main()
{
        gl_Position = P * (MV * vec4(vertex, 1));
        vColor = vec4(0, 0, 0, 1);
}
