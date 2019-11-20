#pragma once

//This header is made so it is easier to work with Ignis
//without having to include everything yourself and manage duplicate resources yourself
//it only provides access to high-level structures

#include "graphics/graphics.hpp"
#include "graphics/surface/swapchain.hpp"
#include "graphics/surface/framebuffer.hpp"
#include "graphics/memory/primitive_buffer.hpp"
#include "graphics/memory/shader_buffer.hpp"
#include "graphics/memory/texture.hpp"
#include "graphics/shader/sampler.hpp"
#include "graphics/shader/pipeline.hpp"
#include "graphics/shader/descriptors.hpp"

namespace igx {

	using namespace ignis::cmd;

	//A wrapper for constructing and destructing graphics objects
	//It creates a GraphicsObject per name; meaning all names should be unique
	//Incase it already exists; it will use the existing one
	template<typename T>
	class GraphicsObjectRef {

		T *ptr{};

	public:

		using Info = typename T::Info;

		void release() {
			if (ptr) {
				ptr->loseRef();
				ptr = nullptr;
			}
		}

		GraphicsObjectRef() {}
		~GraphicsObjectRef() {
			release();
		}

		GraphicsObjectRef(const GraphicsObjectRef &other) {
			if((ptr = other.ptr))
				ptr->addRef();
		}

		GraphicsObjectRef(GraphicsObjectRef &&other) {
			if((ptr = other.ptr))
				ptr->addRef();
		}

		GraphicsObjectRef &operator=(const GraphicsObjectRef &other) {

			release();

			if((ptr = other.ptr))
				ptr->addRef();

			return *this;
		}

		//Maintain equal references (the other one
		GraphicsObjectRef &operator=(GraphicsObjectRef &&other) {

			release();

			if ((ptr = other.ptr) != nullptr)
				ptr->addRef();

			return *this;
		}

		inline T *operator->() { return ptr; }
		inline T *operator->() const { return ptr; }

		inline operator T*() const { return ptr; }
		inline operator T*() { return ptr; }

		//Creates a graphics object if it can't find it
		GraphicsObjectRef(
			ignis::Graphics &g, const String &name, const Info &info
		) {

			//Find existing resource

			auto it = g.find(name);

			if (it != g.end()) {

				if ((ptr = dynamic_cast<T*>(it->second)) != nullptr)
					oic::System::log()->fatal(
						"The requested type existed but had a different type"
					);

				ptr->addRef();
			}

			//Try and create resource (nullptr if it fails)
			
			else try {
				ptr = new T(g, name, info);
			} catch (std::runtime_error err) { }
		}

		//Finds a GraphicsObject
		GraphicsObjectRef(
			ignis::Graphics &g, const String &name
		) {

			//Find existing resource

			auto it = g.find(name);

			if (it != g.end()) {
				ptr = it->second;
				ptr->addRef();
			}
		}

		inline bool exists() const { return ptr; }

		//Use this if you can't use a wrapped object and need the actual pointer
		inline T *get() const { return ptr; }
	};

	//Graphics

	using Graphics = ignis::Graphics;

	//Managed resources

	using CommandList = GraphicsObjectRef<ignis::CommandList>;
	using Swapchain = GraphicsObjectRef<ignis::Swapchain>;
	using Framebuffer = GraphicsObjectRef<ignis::Framebuffer>;
	using PrimitiveBuffer = GraphicsObjectRef<ignis::PrimitiveBuffer>;
	using GPUBuffer = GraphicsObjectRef<ignis::GPUBuffer>;
	using ShaderBuffer = GraphicsObjectRef<ignis::ShaderBuffer>;
	using Pipeline = GraphicsObjectRef<ignis::Pipeline>;
	using Texture = GraphicsObjectRef<ignis::Texture>;
	using Sampler = GraphicsObjectRef<ignis::Sampler>;
	using Descriptors = GraphicsObjectRef<ignis::Descriptors>;

	//Data for info structs

	using BufferAttributes = ignis::BufferAttributes;
	using ShaderBufferLayout = ignis::ShaderBuffer::Layout;
	using BufferLayout = ignis::BufferLayout;
	using PipelineLayout = ignis::PipelineLayout;
	using RegisterLayout = ignis::RegisterLayout;
	using GPUSubresource = ignis::GPUSubresource;

	using GPUFormat = ignis::GPUFormat;
	using DepthFormat = ignis::DepthFormat;
	using GPUBufferType = ignis::GPUBufferType;
	using TextureType = ignis::TextureType;
	using SamplerType = ignis::SamplerType;
	using ShaderAccess = ignis::ShaderAccess;
	using ShaderStage = ignis::ShaderStage;
	using GPUMemoryUsage = ignis::GPUMemoryUsage;
	using PipelineFlag = ignis::Pipeline::Flag;
	using MSAAInfo = ignis::Pipeline::MSAA;

}