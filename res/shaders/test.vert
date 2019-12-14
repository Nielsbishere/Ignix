#version 450

in layout(location=0) vec3 vpos;
out layout(location=0) vec2 uv;

layout(binding=0, std140) uniform Test {
	mat4 p;
	vec3 mask;
};

void main() {
	uv = vpos.xy * 0.5 + 0.5;
	uv.y = 1 - uv.y;
    gl_Position = p * vec4(vpos.xy, vpos.z - 5, 1);
}