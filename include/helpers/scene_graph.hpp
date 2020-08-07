#pragma once
#include "types/scene_object_types.hpp"
#include "factory.hpp"

namespace igx {

	//Scene object type and helpers

	enum class SceneObjectType : u8 {
		TRIANGLE,
		LIGHT,
		MATERIAL,
		CUBE,
		SPHERE,
		PLANE,
		COUNT,
		FIRST = TRIANGLE
	};

	template<SceneObjectType t>
	struct TSceneObjectType_Base {
		static constexpr SceneObjectType type = t;
	};

	template<typename T>
	struct TSceneObjectType : TSceneObjectType_Base<SceneObjectType::COUNT> { };

	template<>
	struct TSceneObjectType<Triangle> : TSceneObjectType_Base<SceneObjectType::TRIANGLE> { };

	template<>
	struct TSceneObjectType<Light> : TSceneObjectType_Base<SceneObjectType::LIGHT> { };

	template<>
	struct TSceneObjectType<Material> : TSceneObjectType_Base<SceneObjectType::MATERIAL> { };

	template<>
	struct TSceneObjectType<Cube> : TSceneObjectType_Base<SceneObjectType::CUBE> { };

	template<>
	struct TSceneObjectType<Sphere> : TSceneObjectType_Base<SceneObjectType::SPHERE> { };

	template<>
	struct TSceneObjectType<Plane> : TSceneObjectType_Base<SceneObjectType::PLANE> { };

	template<typename T>
	static constexpr SceneObjectType SceneObjectType_t = TSceneObjectType<T>::type;

	//Scene graph and info passed to GPU

	union SceneGraphInfo {

		u32 fields[LightType::count + usz(SceneObjectType::COUNT)]{};

		struct {
			u32 objectCount[u8(SceneObjectType::COUNT)];
			u32 lightsCount[LightType::count];
		};

		struct {
			u32 triangleCount, lightCount, materialCount, cubeCount, sphereCount, planeCount;
			u32 directionalLightCount, spotLightCount, pointLightCount;
		};
	};

	class SceneGraph {

	public:

		struct Object {
			GPUBufferRef buffer;
			Buffer cpuData;
			List<bool> markedForUpdate;
			List<u64> toIndex;
		};

		enum class Flags : u32 {
			NONE = 0
		};

	private:

		FactoryContainer &factory;

		Object objects[u8(SceneObjectType::COUNT)];

		HashMap<u64, Pair<SceneObjectType, u32>> indices{};
		u64 counter{};

		SceneGraphInfo *info, limits;
		DescriptorsRef descriptors;
		PipelineLayoutRef layout;
		GPUBufferRef sceneData;

		SamplerRef linear;
		TextureRef skybox;

		Flags flags;

		bool isModified = true;

	public:

		SceneGraph(const SceneGraph&) = delete;
		SceneGraph(SceneGraph&&) = delete;

		SceneGraph &operator=(const SceneGraph&) = delete;
		SceneGraph &operator=(SceneGraph&&) = delete;

		SceneGraph(
			FactoryContainer &factory,
			const String &sceneName,
			const String &skyboxName,
			Flags flags = Flags::NONE,
			u32 maxTriangles = 65536,
			u32 maxLights = 65536,
			u32 maxMaterials = 65536,
			u32 maxCubes = 32768,
			u32 maxSpheres = 16384,
			u32 maxPlanes = 256
		);

		virtual ~SceneGraph() {}

		void del(const List<u64> &ids);

		//Returns an object id; which will keep incrementing
		//This is not the local array index, but rather an identifier that maps to a local index
		//The local object can be moved in memory once previous are moved
		//Returns 0 if it's invalid
		template<typename T>
		inline u64 add(const T &object);

		template<typename T, typename ...args>
		inline void add(const T &object, const args &...arg);

		inline auto find(u64 index) const { return indices.find(index); }
		inline bool exists(u64 index) const { return find(index) != indices.end(); }

		//Update an index 
		//Returns false if invalid index
		template<typename T>
		bool update(u64 index, const T &object);

		//Compact all objects and prepare them for the GPU transfer
		virtual void update(f64 dt);

		//Ensure the copy commands are on the GPU
		void fillCommandList(CommandList *cl);

		//Helpers

		inline auto &getInfo() const { return *info; }
		inline auto &getLimits() const { return limits; }
		inline auto &getSkybox() const { return skybox; }
		inline auto &getDescriptors() const { return descriptors; }
		inline auto &getBuffer(SceneObjectType type) const { return objects[u8(type)].buffer; }

		template<SceneObjectType type>
		inline auto &getBuffer() const { return objects[u8(type)].buffer; }

		static const List<RegisterLayout> &getLayout();

	private:

		//Ensure no gaps are between objects
		void compact(SceneObjectType type);

	};

	//Implementations

	template<typename T>
	inline u64 SceneGraph::add(const T &object) {

		static constexpr SceneObjectType type = SceneObjectType_t<T>;

		static_assert(type != SceneObjectType::COUNT, "Invalid argument passed to SceneGraph::add<T>, expecting a scene object such as Light, Triangle, Cube, etc.");

		do {
			++counter;
		}
		while(!counter || indices.find(counter) != indices.end());

		isModified = true;

		u32 &ind = info->objectCount[u8(type)];
		Object &obj = objects[u8(type)];

		u32 i = 0;

		for (; i < ind; ++i)
			if (!obj.toIndex[i])
				break;

		if (i == ind) {

			if (info->objectCount[u8(type)] == limits.objectCount[u8(type)])
				return 0;

			++ind;
		}

		indices[counter] = { type, i };
		obj.markedForUpdate[i] = true;
		obj.toIndex[i] = counter;

		std::memcpy(obj.cpuData.data() + sizeof(T) * i, &object, sizeof(T));

		return counter;
	}

	template<typename T>
	bool SceneGraph::update(u64 index, const T &object) {

		static constexpr SceneObjectType type = SceneObjectType_t<T>;

		static_assert(type != SceneObjectType::COUNT, "Invalid argument passed to SceneGraph::add<T>, expecting a scene object such as Light, Triangle, Cube, etc.");

		auto it = find(index);

		if (it == indices.end())
			return false;

		if (it->second.first != type) {
			oic::System::log()->error("SceneGraph::update<T> called with incompatible types");
			return false;
		}

		Object &obj = objects[u8(type)];
		u8 *target = obj.cpuData.data() + it->second.second * sizeof(T);

		if (std::memcmp(&object, target, sizeof(T)) == 0)
			return true;

		obj.markedForUpdate[it->second.second] = true;
		std::memcpy(target, &object, sizeof(T));

		return true;
	}

	template<typename T, typename ...args>
	inline void SceneGraph::add(const T &object, const args &...arg) {

		add(object);

		if constexpr (sizeof...(args) > 0)
			add(arg...);

	}

}