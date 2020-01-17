#version 450

//This shader allows Nuklear to be subpixel rendered with gamma correction
//Implemented concepts from puredev software's sub-pixel gamma correct font rendering

in layout(location=0) vec2 uv;
in layout(location=1) vec4 col;

out layout(location=0, index=0) vec4 color;
out layout(location=0, index=1) vec4 blend;

const float gamma = 1.43;
const float invGamma = 1 / gamma;

layout(binding=0, std140) uniform GUIInfo {
	uvec2 res;
	uint subpixelRendering;
};

layout(binding=0) uniform sampler2D atlas;

float doSample(vec2 subpix, vec2 subpixRes) {
	return pow(texture(atlas, subpix / subpixRes).r, gamma);
}

void main() {

	color = col;

	if(subpixelRendering > 0) {

		vec2 subpixRes = res * vec2(3, 1);
		vec2 pos = uv * subpixRes;
		

		blend = vec4(
			doSample(pos + vec2(0, 0), subpixRes) * color.a,
			doSample(pos + vec2(1, 0), subpixRes) * color.a,
			doSample(pos + vec2(2, 0), subpixRes) * color.a,
			color.a
		);
	}

	else {
		float samp = pow(texture(atlas, uv).r, gamma);
		blend = vec4(samp.rrr * color.a, color.a);
	}
}