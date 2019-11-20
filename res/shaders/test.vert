#version 450

in layout(location=0) vec2 vpos;
out layout(location=0) vec2 uv;

void main() {
	uv = vpos * 0.5 + 0.5;
	uv.y = 1 - uv.y;
    gl_Position = vec4(vpos, 0, 1);
}