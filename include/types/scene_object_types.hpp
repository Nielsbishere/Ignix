#pragma once
#include "types/vec.hpp"
#include "types/enum.hpp"
#include "utils/inflect.hpp"
#include "gui/ui_value.hpp"

namespace igx {

	oicExposedEnum(
		LightType, u16, 
		Directional, Spot, Point
	);

	oicExposedEnum(
		ProjectionType, u32, 
		Default, 
		Omnidirectional,
		Stereoscopic_omnidirectional_TB,
		Stereoscopic_TB,
		Stereoscopic_omnidirectional_LR,
		Stereoscopic_LR
	);

	enum class CameraFlags : u32 {
		NONE = 0,
		USE_UI = 1 << 0,
		USE_SUPERSAMPLING = 1 << 1
	};

	enumFlagOverloads(CameraFlags);

	struct Camera {

		Vec3f32 eye{ 4, 2, -2 };
		u32 width;

		Vec3f32 p0;
		u32 height;

		Vec3f32 p1;
		ui::Slider<f32, 50.f, 80.f> ipd = 62;

		Vec3f32 p2;
		ProjectionType projectionType;

		Vec3f32 skyboxColor = Vec3f32(0.25f, 0.5f, 1.f);
		ui::Slider<f32, 0.1f, 4.f> exposure = 1.f;

		Vec3f32 p3;
		f32 focalDistance = 10.f;

		Vec3f32 p4;
		f32 aperature = 0.1f;

		Vec3f32 p5;
		CameraFlags flags = CameraFlags::USE_UI;

		Vec2f32 invRes;
		Vec2u32 tiles;

		InflectWithName(
			{ "Projection type", "Eye", "IPD (mm)", "Skybox color", "Exposure", "Focal distance", "Aperature" },
			projectionType, eye, ipd, skyboxColor, exposure, focalDistance, aperature
		);

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

		Vec3f32 nn = (n.normalize() * 0.5 + 0.5) * u16_MAX;

		return Vec2u32(
			u32(nn.x) << 16 | u32(nn.y),
			nn.z
		);
	}


	struct Light {

		Vec3f32 pos;
		f16 rad, origin;

		Vec2u32 dir;
		f16 r, g, b;
		LightType type;

		Light(Vec3f32 dir, Vec3f32 color) :
			type(LightType::Directional),
			r(color.x), g(color.y), b(color.z),
			dir(encodeNormal(dir))
		{}

		Light(Vec3f32 pos, Vec3f32 color, f32 rad, f32 origin) :
			type(LightType::Point),
			r(color.x), g(color.y), b(color.z),
			pos(pos), rad(rad), origin(origin)
		{}

	};

	struct Material {

		f16 albedoR, albedoG, albedoB; u16 metallic;
		f16 ambientR, ambientG, ambientB; u16 roughness;

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
			metallic(u16(metallic * u16_MAX)), roughness(u16(roughness * u16_MAX)), 
			transparency(transparency)
		{}

	};

}