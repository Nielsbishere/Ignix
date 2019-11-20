#version 450

in layout(location=0) vec2 uv;
out layout(location=0) vec4 color;

layout(binding=0) uniform sampler2D test;

layout(binding=0, std140) uniform Test {
	vec3 mask;
};

void main() {
    color = vec4(texture(test, uv).rgb * mask, 1);
}