#pragma once

//This header is made so it is easier to work with Ignis
//without having to include everything yourself and manage duplicate resources yourself
//it only provides access to high-level structures

#include "graphics/graphics.hpp"
#include "graphics/memory/swapchain.hpp"
#include "graphics/memory/framebuffer.hpp"
#include "graphics/memory/primitive_buffer.hpp"
#include "graphics/memory/shader_buffer.hpp"
#include "graphics/memory/texture.hpp"
#include "graphics/shader/sampler.hpp"
#include "graphics/shader/pipeline.hpp"
#include "graphics/shader/descriptors.hpp"
#include "graphics/command/commands.hpp"

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
		using Ptr = T*;

		void release() {
			if (ptr) {
				ptr->loseRef();
				ptr = nullptr;
			}
		}

		GraphicsObjectRef() { static_assert(std::is_base_of_v<ignis::GPUObject, T>, "GraphicsObjectRef can only be used on GraphicsObjects");  }
		~GraphicsObjectRef() {
			release();
		}

		GraphicsObjectRef(Ptr ptr): ptr(ptr) {
			if(ptr)
				ptr->addRef();
		}

		GraphicsObjectRef(const GraphicsObjectRef &other) {
			if((ptr = other.ptr) != nullptr)
				ptr->addRef();
		}

		GraphicsObjectRef(GraphicsObjectRef &&other) {
			ptr = other.ptr;
			other.ptr = nullptr;
		}

		GraphicsObjectRef &operator=(const GraphicsObjectRef &other) {

			release();

			if((ptr = other.ptr) != nullptr)
				ptr->addRef();

			return *this;
		}

		//Maintain equal references (the other one
		GraphicsObjectRef &operator=(GraphicsObjectRef &&other) noexcept {
			ptr = other.ptr;
			other.ptr = nullptr;
			return *this;
		}

		inline T *operator->() { return ptr; }
		inline T *operator->() const { return ptr; }

		inline operator T*() const { return ptr; }
		inline operator T*() { return ptr; }

		//Creates a graphics object
		GraphicsObjectRef(
			ignis::Graphics &g, const String &name, const Info &info
		) {

			//Find existing resource

			auto it = g.find(name);

			oicAssert("The requested resource already exists", it == g.endByName());

			//Try and create resource (nullptr if it fails)
			
			try {
				ptr = new T(g, name, info);
			} catch (std::runtime_error&) { }
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
	using DescriptorsSubresources = ignis::Descriptors::Subresources;
	using RegisterLayout = ignis::RegisterLayout;
	using GPUSubresource = ignis::GPUSubresource;
	using Rasterizer = ignis::Rasterizer;
	using BlendState = ignis::BlendState;
	using MSAA = ignis::MSAA;
	using DepthStencil = ignis::DepthStencil;

	using GPUFormat = ignis::GPUFormat;
	using DepthFormat = ignis::DepthFormat;
	using GPUBufferType = ignis::GPUBufferType;
	using TextureType = ignis::TextureType;
	using SamplerType = ignis::SamplerType;
	using ShaderAccess = ignis::ShaderAccess;
	using ShaderStage = ignis::ShaderStage;
	using GPUMemoryUsage = ignis::GPUMemoryUsage;
	using PipelineFlag = ignis::Pipeline::Flag;
	using TopologyMode = ignis::TopologyMode;
	using FillMode = ignis::FillMode;
	using CullMode = ignis::CullMode;
	using WindMode = ignis::WindMode;
	using BlendLogicOp = ignis::BlendState::LogicOp;
	using Stencil = ignis::DepthStencil::Stencil;

}

namespace std {

	template<typename T>
	struct hash<igx::GraphicsObjectRef<T>> {

		inline usz operator()(const igx::GraphicsObjectRef<T> &t) const {
			return usz(t.get());
		}

	};

}