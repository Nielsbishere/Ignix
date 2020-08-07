#pragma once
#include "types/vec.hpp"
#include "types/enum.hpp"

namespace igx {

	oicExposedEnum(
		LightType, u16, 
		Directional, Spot, Point
	);

	oicExposedEnum(
		ProjectionType, u32, 
		Default, 
		Omnidirectional,
		Equirect,
		Stereoscopic_omnidirectional_TB,
		Stereoscopic_TB,
		Stereoscopic_omnidirectional_LR,
		Stereoscopic_LR
	);

	struct Camera {

		Vec3f32 eye{ 4, 2, -2 };
		u32 width;

		Vec3f32 p0;
		u32 height;

		Vec3f32 p1;
		f32 ipd = 6.2f;

		Vec3f32 p2;
		ProjectionType projectionType;

		Vec3f32 skyboxColor = Vec3f32(0.25f, 0.5f, 1.f);
		f32 exposure = 1.f;

		Vec2f32 invRes;
		f32 focalDistance = 10.f;
		f32 aperature = 0.1f;

	};

	struct Triangle {
		Vec3f32 p0, p1, p2;
	};

	struct Cube {
		Vec3f32 min, max;
	};

	struct Sphere {
		Vec3f32 pos;
		f32 rad;
	};

	struct Plane {
		Vec3f32 dir;
		f32 dist;
	};

	static constexpr inline Vec2u32 encodeNormal(Vec3f32 n) {

		n = n * 0.5 + 0.5;

		Vec3u32 multi = (n * (Vec3f32(1 << 22, 1 << 22, 1 << 20) - 1)).cast<Vec3u32>();

		return Vec2u32(
			(multi.x << 10) | (multi.y >> 12),
			(multi.y << 20) | multi.z
		);
	}


	struct Light {

		Vec3f32 pos;
		f16 rad, origin;

		Vec2u32 dir;
		f16 r, g, b;
		LightType lightType;

		Light(Vec3f32 dir, Vec3f32 color) :
			lightType(LightType::Directional),
			r(color.x), g(color.y), b(color.z),
			dir(encodeNormal(dir))
		{}

		Light(Vec3f32 pos, Vec3f32 color, f32 rad, f32 origin) :
			lightType(LightType::Point),
			r(color.x), g(color.y), b(color.z),
			pos(pos), rad(rad), origin(origin)
		{}

	};

	struct Material {

		f16 albedoR, albedoG, albedoB, metallic;
		f16 ambientR, ambientG, ambientB, roughness;

		f16 emissionR, emissionG, emissionB, pad2;
		f32 transparency;
		u32 materialInfo{};

		Material(
			Vec3f32 albedo, 
			Vec3f32 ambient, 
			Vec3f32 emission, 
			f32 metallic, f32 roughness,
			f32 transparency
		):
			albedoR(albedo.x), albedoG(albedo.y), albedoB(albedo.z),
			ambientR(ambient.x), ambientG(ambient.y), ambientB(ambient.z),
			emissionR(emission.x), emissionG(emission.y), emissionB(emission.z),
			metallic(metallic), roughness(roughness), transparency(transparency)
		{}

	};

}