#pragma once
#include "common.hpp"

namespace igx {

	template<typename T>
	class Factory {

		Graphics &g;
		List<GraphicsObjectRef<T>> objects;

	public:

		//TODO: Problem, factory should be notified when objects aren't referenced anymore

		Factory(Graphics &g): g(g) {}
		~Factory() {}

		Factory(const Factory&) = delete;
		Factory(Factory&&) = delete;
		Factory &operator=(const Factory&) = delete;
		Factory &operator=(Factory&&) = delete;

		inline Graphics &getGraphics() const { return g; }

		inline auto get(const String &name, const typename T::Info &initInfo) {

			for (auto &object : objects)
				if (object->getInfo() == initInfo)
					return object;

			GraphicsObjectRef<T> object {
				g, name,
				initInfo
			};

			objects.push_back(object);
			return object;
		}

	};

	using PipelineFactory = Factory<Pipeline>;
	using PipelineLayoutFactory = Factory<PipelineLayout>;
	using SamplerFactory = Factory<Sampler>;

	class FactoryContainer {

		PipelineFactory pipelines;
		PipelineLayoutFactory pipelineLayouts;
		SamplerFactory samplers;

		UploadBufferRef defaultUploadBuffer;

	public:

		FactoryContainer(Graphics &g) :
			pipelines(g), pipelineLayouts(g), samplers(g),
			defaultUploadBuffer(
				g, NAME("Factory container upload buffer"),
				UploadBuffer::Info(
					1_MiB, 1_MiB, 64_MiB
				)
			)
		{ }

		inline Graphics &getGraphics() const { return pipelines.getGraphics(); }
		inline UploadBufferRef getDefaultUploadBuffer() const { return defaultUploadBuffer; }

		inline auto get(const String &name, const Pipeline::Info &p) { 
			return pipelines.get(name, p); 
		}

		inline auto get(const String &name, const PipelineLayout::Info &p) { 
			return pipelineLayouts.get(name, p); 
		}

		inline auto get(const String &name, const Sampler::Info &s) { 
			return samplers.get(name, s); 
		}

	};

}