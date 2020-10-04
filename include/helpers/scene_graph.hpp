#pragma once
#include "types/scene_object_types.hpp"
#include "factory.hpp"
#include "gui/gui.hpp"
#include "gui/struct_inspector.hpp"

namespace igx {

	//Scene object type and helpers

	enum class SceneObjectType : u8 {
		LIGHT,
		MATERIAL,
		TRIANGLE,
		SPHERE,
		CUBE,
		PLANE,
		COUNT,
		FIRST = LIGHT
	};

	template<SceneObjectType t, bool _isGeometry, bool _isValid = true>
	struct TSceneObjectType_Base {
		static constexpr SceneObjectType type = t;
		static constexpr bool isGeometry = _isGeometry;
		static constexpr bool isValid = _isValid;
	};

	template<typename T>
	struct TSceneObjectType : TSceneObjectType_Base<SceneObjectType::COUNT, false, false> { };

	template<>
	struct TSceneObjectType<Triangle> : TSceneObjectType_Base<SceneObjectType::TRIANGLE, true> { };

	template<>
	struct TSceneObjectType<Light> : TSceneObjectType_Base<SceneObjectType::LIGHT, false> { };

	template<>
	struct TSceneObjectType<Material> : TSceneObjectType_Base<SceneObjectType::MATERIAL, false> { };

	template<>
	struct TSceneObjectType<Cube> : TSceneObjectType_Base<SceneObjectType::CUBE, true> { };

	template<>
	struct TSceneObjectType<Sphere> : TSceneObjectType_Base<SceneObjectType::SPHERE, true> { };

	template<>
	struct TSceneObjectType<Plane> : TSceneObjectType_Base<SceneObjectType::PLANE, true> { };

	template<typename T>
	static constexpr SceneObjectType SceneObjectType_t = TSceneObjectType<T>::type;

	template<typename T>
	static constexpr bool SceneObjectTypeIsGeometry = TSceneObjectType<T>::isGeometry;

	template<typename T>
	static constexpr bool SceneObjectTypeIsValid = TSceneObjectType<T>::isValid;

	//Scene graph and info passed to GPU

	union SceneGraphInfo {

		u32 fields[LightType::count + usz(SceneObjectType::COUNT)]{};

		struct {
			u32 objectCount[u8(SceneObjectType::COUNT)];
			u32 lightsCount[LightType::count];
		};

		struct {
			u32 lightCount, materialCount, triangleCount, sphereCount, cubeCount, planeCount;
			u32 directionalLightCount, spotLightCount, pointLightCount;
		};
	};

	class SceneGraph {

	public:

		struct Inspection;

		struct Object {
			GPUBufferRef buffer;
			Buffer cpuData;
			List<bool> markedForUpdate;
			List<u64> toIndex;
		};

		struct Entry {
			u32 index, material;
			SceneObjectType type;
		};

		enum class Flags : u32 {
			NONE = 0
		};

	private:

		ui::GUI &gui;
		FactoryContainer &factory;

		Object objects[u8(SceneObjectType::COUNT)];

		HashMap<u64, Entry> entries{};
		u64 counter{};

		SceneGraphInfo *info, limits;
		DescriptorsRef descriptors;
		PipelineLayoutRef layout;
		GPUBufferRef sceneData, materialIndices;

		SamplerRef linear;
		TextureRef skybox;

		u32 geometryId{};
		u32 *materialByObject{};
		Flags flags;

		void *inspector;

		bool isModified = true;

	public:

		SceneGraph(const SceneGraph&) = delete;
		SceneGraph(SceneGraph&&) = delete;

		SceneGraph &operator=(const SceneGraph&) = delete;
		SceneGraph &operator=(SceneGraph&&) = delete;

		SceneGraph(
			ui::GUI &gui,
			FactoryContainer &factory,
			const String &sceneName,
			const String &skyboxName,
			u32 maxTriangles = 65536,
			u32 maxLights = 65536,
			u32 maxMaterials = 65536,
			u32 maxCubes = 32768,
			u32 maxSpheres = 16384,
			u32 maxPlanes = 256,
			Flags flags = Flags::NONE
		);

		virtual ~SceneGraph();

		void del(const List<u64> &ids);

		//Add non geometry objects
		//Returns an object id; which will keep incrementing
		//This is not the local array index, but rather an identifier that maps to a local index
		//The local object can be moved in memory once previous are moved
		//Returns 0 if it's invalid
		template<typename T>
		inline u64 addNonGeometry(const T &object);

		//Add geometry objects (with certain materials)
		//Returns an object id; which will keep incrementing
		//This is not the local array index, but rather an identifier that maps to a local index
		//The local object can be moved in memory once previous are moved
		//Returns 0 if it's invalid
		template<typename T>
		inline u64 addGeometry(const T &object, const u32 material);

		template<typename T, typename T2, typename ...args>
		inline void add(const T &obj0, const T2 &obj1, const args &...arg);

		template<typename T>
		inline void add(const T &object);

		inline auto find(u64 index) const { return entries.find(index); }
		inline bool exists(u64 index) const { return find(index) != entries.end(); }

		virtual void input(const oic::InputDevice*, oic::InputHandle, bool) {}

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

		u64 addInternal(SceneObjectType type, const void *obj, usz siz, u32 material);

		//Ensure no gaps are between objects
		void compact(SceneObjectType type);

	};

	//Implementations

	template<typename T>
	inline u64 SceneGraph::addNonGeometry(const T &object) {

		static constexpr SceneObjectType type = SceneObjectType_t<T>;

		static_assert(type != SceneObjectType::COUNT, "Invalid argument passed to SceneGraph::add<T>, expecting a scene object such as Light, Triangle, Cube, etc.");

		static_assert(!SceneObjectTypeIsGeometry<T>, "Geometry requires Pair<T geometryType, u32 material> in SceneGraph::add");

		return addInternal(type, &object, sizeof(T), 0);
	}

	template<typename T>
	inline u64 SceneGraph::addGeometry(const T &object, const u32 material) {

		static constexpr SceneObjectType type = SceneObjectType_t<T>;

		static_assert(type != SceneObjectType::COUNT, "Invalid argument passed to SceneGraph::add<T>, expecting a scene object such as Light, Triangle, Cube, etc.");

		static_assert(SceneObjectTypeIsGeometry<T>, "Geometry is only allowed with Pair<T, u32> in SceneGraph::add");

		return addInternal(type, &object, sizeof(T), material);
	}

	template<typename T>
	bool SceneGraph::update(u64 index, const T &object) {

		static constexpr SceneObjectType type = SceneObjectType_t<T>;

		static_assert(type != SceneObjectType::COUNT, "Invalid argument passed to SceneGraph::add<T>, expecting a scene object such as Light, Triangle, Cube, etc.");

		auto it = find(index);

		if (it == entries.end())
			return false;

		if (it->second.type != type) {
			oic::System::log()->error("SceneGraph::update<T> called with incompatible types");
			return false;
		}

		Object &obj = objects[u8(type)];
		u8 *target = obj.cpuData.data() + it->second.index * sizeof(T);

		if (std::memcmp(&object, target, sizeof(T)) == 0)
			return true;

		obj.markedForUpdate[it->second.index] = true;
		std::memcpy(target, &object, sizeof(T));

		return true;
	}

	template<typename T, typename T2, typename ...args>
	inline void SceneGraph::add(const T &obj0, const T2 &obj1, const args &...arg) {

		if constexpr (SceneObjectTypeIsGeometry<T>) {

			static_assert(std::is_same_v<T2, u32>, "Geometry requires a u32 material right after it");

			addGeometry(obj0, obj1);

			if constexpr (sizeof...(args) > 0)
				add(arg...);
		}

		else {
			addNonGeometry(obj0);
			add(obj1, arg...);
		}
	}

	template<typename T>
	inline void SceneGraph::add(const T &obj) {
		addNonGeometry(obj);
	}

}