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

		Vec3f32 Position;
		ui::Slider<f32, 0.1f, 100.f> Radius;

		Inflect(Position, Radius);
	};

	struct Plane {
		Vec3f32 dir;
		f32 dist;
	};

	static constexpr inline Vec2u32 encodeNormal(const Vec3f32 &n) {

		Vec3f32 nn = (n.normalize() * 0.5 + 0.5) * u16_MAX;

		return Vec2u32(
			u32(nn.x) << 16 | u32(nn.y),
			nn.z
		);
	}

	static constexpr inline Vec3f32 decodeNormal(const Vec2u32 &e) {
		Vec3f32 nn = Vec3f32(f32(e.x >> 16), u16(e.x), e.y) / u16_MAX;
		return nn * 2 - 1;
	}

	static constexpr inline Vec3f32 fromPolar(const Vec2f32 &polar) {
		auto cosPolar = polar.cos(), sinPolar = polar.sin();
		return Vec3f32(sinPolar.x * cosPolar.y, sinPolar.x * sinPolar.y, cosPolar.x);
	}

	static constexpr inline Vec2f32 toPolar(const Vec3f32 &n) {
		return {
			std::acos(n.z),
			std::atan2(-n.x, n.y) + oic::Math::PI0_5
		};
	}

	struct Light {

		Vec3f32 pos;
		f16 rad, origin;

		Vec2u32 dir;
		f16 r, g, b;
		LightType type;

		Light(Vec3f32 dir, Vec3f32 color, f32 angularExtent = 0.533_deg) :
			type(LightType::Directional),
			r(color.x), g(color.y), b(color.z),
			dir(encodeNormal(dir)), rad(angularExtent)
		{}

		Light(Vec3f32 pos, Vec3f32 color, f32 rad, f32 origin, f32 specularity = 1) :
			type(LightType::Point),
			r(color.x), g(color.y), b(color.z),
			pos(pos), rad(rad), origin(origin),
			dir(*(u32*) &specularity, 0)
		{}

		InflectBody(

			using ColorSlider = ui::Slider<f32, 0.f, 3.f>;
			ColorSlider _r = r, _g = g, _b = b;

			switch (type.value) {

				//Directional light inspector

				case LightType::Directional: {

					using AngularSlider = ui::Slider<f32, 0.1_deg, 2_deg>;
					using DirSlider = ui::Slider<f32, 0, 360>;

					Vec3f32 d = decodeNormal(dir);
					Vec2f32 thetaPhi = toPolar(d).radToDeg();
					AngularSlider ae = rad;

					static const List<String> namesOfArgs = { "Type", "Theta", "Phi", "R", "G", "B", "Angular extent" };

					//Const

					if constexpr(std::is_const_v<decltype(*this)>)
						inflector.inflect(
							this, recursion, namesOfArgs, type, 
							(const DirSlider&) thetaPhi.x, 
							(const DirSlider&) thetaPhi.y, 
							(const ColorSlider&) _r, (const ColorSlider&) _g, (const ColorSlider&) _b, 
							(const AngularSlider&) ae
						);

					//Non const

					else {

						inflector.inflect(
							this, recursion, namesOfArgs, type, 
							(DirSlider&) thetaPhi.x, (DirSlider&) thetaPhi.y, _r, _g, _b, ae
						);	

						r = _r.value; g = _g.value; b = _b.value;
						rad = ae.value;
						dir = encodeNormal(fromPolar(thetaPhi.degToRad()));
					}

					return;
				}
			}

			//Point light

			using RadSlider = ui::Slider<f32, 0.05f, 100.f>;
			using LightSpecular = ui::Slider<f32, 0.25f, 64.f>;

			RadSlider _rad = rad, _origin = origin;
			
			static const List<String> namesOfArgs = { 
				"Type", "Position", "Radius", "Origin radius", "R", "G", "B", "Light specularity"
			};

			//Const

			if constexpr(std::is_const_v<decltype(*this)>)
				inflector.inflect(
					this, recursion, namesOfArgs, type, pos, 
					(const RadSlider&) _rad, (const RadSlider&) _origin, 
					(const ColorSlider&) _r, (const ColorSlider&) _g, (const ColorSlider&) _b,
					(const LightSpecular&) dir.x
				);	

			//Non const

			else {

				inflector.inflect(
					this, recursion, namesOfArgs, type, pos, _rad, _origin, _r, _g, _b,
					(LightSpecular&) dir.x
				);

				r = _r.value; g = _g.value; b = _b.value;
				rad = _rad.value;
				origin = _origin.value;
			}
		)

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