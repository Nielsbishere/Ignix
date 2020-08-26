#include "helpers/scene_graph.hpp"
#include "igxi/convert.hpp"

namespace igx {

	static String sceneObjectNames[u8(SceneObjectType::COUNT)] = {
		" lights",
		" materials",
		" triangles",
		" spheres",
		" cubes",
		" planes"
	};

	static constexpr usz sceneObjectStrides[u8(SceneObjectType::COUNT)] = {
		sizeof(Light),
		sizeof(Material),
		sizeof(Triangle),
		sizeof(Sphere),
		sizeof(Cube),
		sizeof(Plane)
	};

	SceneGraph::SceneGraph(
		FactoryContainer &factory,
		const String &sceneName,
		const String &skyboxName,
		u32 maxTriangles,
		u32 maxLights,
		u32 maxMaterials,
		u32 maxCubes,
		u32 maxSpheres,
		u32 maxPlanes,
		Flags flags
	):
		factory(factory),
		flags(flags),
		limits {
			{
				maxTriangles,
				maxLights,
				maxMaterials,
				maxCubes,
				maxSpheres,
				maxPlanes
			}
		}
	{
		if(skyboxName.size())
			skybox = {
				factory.getGraphics(), NAME(sceneName + " skybox"),
				igxi::Helper::loadDiskExternal(skyboxName, factory.getGraphics())
			};

		u32 totalObjectCount{};

		for (SceneObjectType type = SceneObjectType::FIRST; type != SceneObjectType::COUNT; type = SceneObjectType(u8(type) + 1)) {

			u32 objectCount = limits.objectCount[u8(type)];

			if (objectCount == 0)
				continue;

			oicAssert("Only up to 4B primitives supported in scene graph", totalObjectCount < totalObjectCount + objectCount);

			totalObjectCount += objectCount;

			objects[u8(type)] = Object {
				GPUBufferRef(
					factory.getGraphics(), NAME(sceneName + sceneObjectNames[u8(type)]),
					GPUBuffer::Info(
						objectCount * sceneObjectStrides[u8(type)], GPUBufferType::STRUCTURED,
						GPUMemoryUsage::CPU_WRITE
					)
				),
				Buffer(objectCount * sceneObjectStrides[u8(type)]),
				List<bool>(objectCount),
				List<u64>(objectCount)
			};
		}

		materialIndices = {
			factory.getGraphics(), NAME("Scene material indices"),
			GPUBuffer::Info(
				totalObjectCount * sizeof(u32), GPUBufferType::STRUCTURED,
				GPUMemoryUsage::CPU_WRITE
			)
		};

		materialByObject = (u32*) materialIndices->getBuffer();

		linear = factory.get(NAME("Linear sampler"), Sampler::Info(
			SamplerMin::LINEAR, SamplerMag::LINEAR, SamplerMode::CLAMP_BORDER, 1.f
		));

		layout = factory.get(NAME("Scene graph layout"), PipelineLayout::Info(
			getLayout()
		));

		sceneData = {
			factory.getGraphics(), NAME("Scene data"),
			GPUBuffer::Info(
				sizeof(SceneGraphInfo), GPUBufferType::UNIFORM, GPUMemoryUsage::CPU_WRITE
			)
		};

		info = (SceneGraphInfo*) sceneData->getBuffer();

		descriptors = {
			factory.getGraphics(), NAME(sceneName + " descriptors"),
			Descriptors::Info(
				layout, 1, Descriptors::Subresources{
					{ 1, GPUSubresource(sceneData) },
					{ 2, GPUSubresource(objects[u8(SceneObjectType::TRIANGLE)].buffer) },
					{ 3, GPUSubresource(objects[u8(SceneObjectType::SPHERE)].buffer) },
					{ 4, GPUSubresource(objects[u8(SceneObjectType::CUBE)].buffer) },
					{ 5, GPUSubresource(objects[u8(SceneObjectType::PLANE)].buffer) },
					{ 6, GPUSubresource(objects[u8(SceneObjectType::LIGHT)].buffer) },
					{ 7, GPUSubresource(objects[u8(SceneObjectType::MATERIAL)].buffer) },
					{ 8, GPUSubresource(materialIndices) },
					{ 9, GPUSubresource(linear, skybox, TextureType::TEXTURE_2D) }
				}
			)
		};

	}

	const List<RegisterLayout> &SceneGraph::getLayout() {
		
		static const List<RegisterLayout> layout = {

			//Uniforms

			RegisterLayout(
				NAME("CameraData"), 0, GPUBufferType::UNIFORM, 0, 0,
				ShaderAccess::COMPUTE, sizeof(Camera)
			),

			RegisterLayout(
				NAME("SceneData"), 1, GPUBufferType::UNIFORM, 1, 1,
				ShaderAccess::COMPUTE, sizeof(SceneGraphInfo)
			),

			//SSBOs

			RegisterLayout(
				NAME("Triangles"), 2, GPUBufferType::STRUCTURED, 0, 1,
				ShaderAccess::COMPUTE, sizeof(Triangle)
			),

			RegisterLayout(
				NAME("Spheres"), 3, GPUBufferType::STRUCTURED, 1, 1,
				ShaderAccess::COMPUTE, sizeof(Sphere)
			),

			RegisterLayout(
				NAME("Cubes"), 4, GPUBufferType::STRUCTURED, 2, 1,
				ShaderAccess::COMPUTE, sizeof(Cube)
			),

			RegisterLayout(
				NAME("Planes"), 5, GPUBufferType::STRUCTURED, 3, 1,
				ShaderAccess::COMPUTE, sizeof(Plane)
			),

			RegisterLayout(
				NAME("Lights"), 6, GPUBufferType::STRUCTURED, 4, 1,
				ShaderAccess::COMPUTE, sizeof(Light)
			),

			RegisterLayout(
				NAME("Materials"), 7, GPUBufferType::STRUCTURED, 5, 1,
				ShaderAccess::COMPUTE, sizeof(Material)
			),

			RegisterLayout(
				NAME("Material indices"), 8, GPUBufferType::STRUCTURED, 6, 1,
				ShaderAccess::COMPUTE, sizeof(u32)
			),

			//Skybox

			RegisterLayout(
				NAME("Skybox"), 9, SamplerType::SAMPLER_2D, 0, 1,
				ShaderAccess::COMPUTE
			)
		};

		return layout;
	}

	void SceneGraph::fillCommandList(CommandList *cl) {

		cl->add(
			FlushImage(skybox, factory.getDefaultUploadBuffer()),
			FlushBuffer(sceneData, factory.getDefaultUploadBuffer()),
			FlushBuffer(materialIndices, factory.getDefaultUploadBuffer())
		);

		for (auto &obj : objects)
			cl->add(
				FlushBuffer(obj.buffer, factory.getDefaultUploadBuffer())
			);
	}

	void SceneGraph::update(f64) {

		geometryId = 0;

		for (SceneObjectType type = SceneObjectType::FIRST; type != SceneObjectType::COUNT; type = SceneObjectType(u8(type) + 1)) {

			//Ensure it's all one array

			compact(type);

			//Flush regions that are modified to gpu

			Object &obj = objects[u8(type)];
			usz stride = sceneObjectStrides[u8(type)];

			u32 prevMarked = u32_MAX, i = 0;

			for (; i < info->objectCount[u8(type)]; ++i)

				if (prevMarked == u32_MAX) {

					if (obj.markedForUpdate[i])
						prevMarked = i;
				}

				else if (!obj.markedForUpdate[i]) {

					//Ensure data is in our other cpu copy
					//Not our intermediate

					std::memcpy(
						obj.buffer->getBuffer() + stride * prevMarked,
						obj.cpuData.data() + stride * prevMarked,
						(i - prevMarked) * stride
					);

					obj.buffer->flush(stride * prevMarked, (i - prevMarked) * stride);
				}

			//Left over

			if (prevMarked != usz_MAX) {

				std::memcpy(
					obj.buffer->getBuffer() + stride * prevMarked,
					obj.cpuData.data() + stride * prevMarked,
					(i - prevMarked) * stride
				);

				obj.buffer->flush(stride * prevMarked, (i - prevMarked) * stride);
			}
		}

		//It's just a few bytes, can be flushed, the check isn't really needed

		sceneData->flush(0, sizeof(*info));
	}

	void SceneGraph::del(const List<u64> &ids) {
	
		for (u64 i : ids) {

			auto it = find(i);

			if (it == entries.end())
				continue;

			objects[u8(it->first)].toIndex[it->second.index] = 0;
			objects[u8(it->first)].markedForUpdate[it->second.index] = false;
			entries.erase(it);
		}

	}

	void SceneGraph::compact(SceneObjectType type) {

		u32 j{};

		auto &obj = objects[u8(type)];
		usz stride = sceneObjectStrides[u8(type)];
		u32 &count = info->objectCount[u8(type)];

		u8 *cpuPtr = obj.cpuData.data();
		u8 *gpuPtr = obj.buffer->getBuffer();

		bool needsRemap{};

		switch (type) {
			
			//Light has to sort by type as well as eliminate dead space

			case SceneObjectType::LIGHT: {

				u32 counters[LightType::count]{}, counters0[LightType::count]{};
				Light *lc = (Light*) cpuPtr;

				//Detect if dead space exists or indices changed (e.g. light type changed)
				//otherwise don't mark dirty
				//Also count light types

				for (u32 i = 0; i < count; ++i) {

					if (!obj.toIndex[i]) {
						needsRemap = true;
						continue;
					}

					++j;
					++counters[lc[i].type.value];
				}

				for (usz i = 0; i < LightType::count; ++i)
					if (info->lightsCount[i] != counters[i]) {
						info->lightsCount[i] = counters[i];
						needsRemap = true;
					}

				if (!needsRemap)
					return;

				//Remap to ensure our dead space doesn't exist on the gpu
				//And to make sure the point lights are after spot and after directional lights

				for (u32 i = 0; i < count; ++i) {

					u64 id = obj.toIndex[i];

					if (!id)
						continue;

					usz lightTypeId = usz(lc[i].type.value);
					u32 &localId = counters0[lightTypeId];

					u32 globalId = localId;

					for (usz k = 0; k < lightTypeId; ++k)
						globalId += counters[k];

					++localId;

					obj.toIndex[globalId] = id;

					if (entries[id].index != globalId) {
						obj.markedForUpdate[i] = false;
						obj.markedForUpdate[globalId] = true;
						entries[id].index = globalId;
					}

					std::memcpy(gpuPtr + globalId * stride, cpuPtr + i * stride, stride);
				}

				break;
			}

			//Normal types just need to eliminate dead space

			default:

				//Detect if dead space exists, otherwise don't mark dirty

				for (u32 i = 0; i < count; ++i) {

					u64 id = obj.toIndex[i];

					//TODO: Update all geometry if the material ids change

					if (!id) {

						//TODO: Change material of geometries to 0

						needsRemap = true;
						continue;
					}

					if (type != SceneObjectType::MATERIAL) {

						u32 &dst = materialByObject[geometryId];
						u32 src = entries[id].material;

						if (dst != src) {
							dst = src;
							materialIndices->flush(geometryId * sizeof(u32), sizeof(u32));
						}

						++geometryId;
					}
				}

				if (!needsRemap)
					return;

				//Remap to ensure our dead space doesn't exist on the gpu

				for (u32 i = 0; i < count; ++i) {

					u64 id = obj.toIndex[i];

					if (!id)
						continue;

					obj.toIndex[j] = id;

					if (entries[id].index != j) {
						obj.markedForUpdate[i] = false;
						obj.markedForUpdate[j] = true;
						entries[id].index = j;
					}

					std::memcpy(gpuPtr + j * stride, cpuPtr + i * stride, stride);
					++j;
				}
		}


		//Keep the same order on the CPU as well

		count = j;
		std::memcpy(cpuPtr, gpuPtr, j * stride);
	}

}