#version 330 core

in vec2 coords;
out vec4 col;
uniform sampler2D screenTexture;

void main() {
    float off = 300.0;
    vec4 c1 = 1 *texture(screenTexture, coords+vec2(-1.0, -1.0)/off);
    vec4 c2 = 1  * texture(screenTexture, coords+vec2(-1.0,  0.0)/off);
    vec4 c3 = 1 * texture(screenTexture, coords+vec2(-1.0,  1.0)/off);
    vec4 c4 = 1  * texture(screenTexture, coords+vec2( 0.0, -1.0)/off);
    vec4 c5 = -7   * texture(screenTexture, coords+vec2( 0.0,  0.0)/off);
    vec4 c6 = 1  * texture(screenTexture, coords+vec2( 0.0,  1.0)/off);
    vec4 c7 = 1 * texture(screenTexture, coords+vec2( 1.0, -1.0)/off);
    vec4 c8 = 1 * texture(screenTexture, coords+vec2( 1.0,  0.0)/off);
    vec4 c9 = 1 * texture(screenTexture, coords+vec2( 1.0,  1.0)/off);
    vec4 sum = (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);
    //col = sum;
    col = texture(screenTexture, coords);
    //col = vec4(1, 1, 1, 1);
}
