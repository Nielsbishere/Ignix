#version 450

in layout(location=0) vec2 vpos;
in layout(location=1) vec2 vuv;
in layout(location=2) vec4 vcol;
out layout(location=0) vec2 uv;
out layout(location=1) vec4 col;

layout(binding=0, std140) uniform Res {
	uvec2 res;
};

void main() {
	uv = vuv;
	col = vcol;
    gl_Position = vec4(vpos / (res - 1) * 2 - 1, 0, 1);
}