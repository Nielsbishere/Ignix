#version 450

in layout(location=0) vec2 uv;
in layout(location=1) vec3 vcol;
out layout(location=0) vec4 color;

layout(binding=0) uniform sampler2D test;

layout(binding=0, std140) uniform Test {
	mat4 pvw;
	vec3 mask;
};

void main() {
    color = vec4(texture(test, uv).rgb * vcol * mask, 1);
}