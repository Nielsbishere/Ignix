#version 450

in layout(location=0) vec2 uv;
in layout(location=1) vec4 col;

out layout(location=0) vec4 color;

layout(binding=0) uniform sampler2D image;

void main() {
	float outline = texture(image, uv).r;
    color = outline.rrrr * col;
}