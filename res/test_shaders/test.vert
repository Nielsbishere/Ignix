#version 450

in layout(location=0) vec3 mpos;
in layout(location=1) vec2 vuv;
out layout(location=0) vec2 uv;
out layout(location=1) vec3 col;

layout(binding=0, std140) uniform Test {
	mat4 pvw;
	vec3 mask;
};

void main() {
	uv = vuv;
	col = vec3(uv, 1);
    gl_Position = pvw * vec4(mpos, 1);
}