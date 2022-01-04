#version 330 core
in vec2 pos;
out vec2 coords;
uniform float time;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    coords = (pos+vec2(1,1))/2.0;
}
