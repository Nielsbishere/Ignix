#pragma once

//This header is made so it is easier to work with Ignis
//without having to include everything yourself and manage duplicate resources yourself
//it only provides access to high-level structures

#include "graphics/graphics.hpp"
#include "graphics/memory/swapchain.hpp"
#include "graphics/memory/framebuffer.hpp"
#include "graphics/memory/primitive_buffer.hpp"
#include "graphics/memory/shader_buffer.hpp"
#include "graphics/memory/upload_buffer.hpp"
#include "graphics/memory/texture.hpp"
#include "graphics/shader/sampler.hpp"
#include "graphics/shader/pipeline.hpp"
#include "graphics/shader/descriptors.hpp"
#include "graphics/command/commands.hpp"

namespace igx {

	using namespace ignis::cmd;
	using namespace ignis;

	//A wrapper for constructing and destructing graphics objects
	//It creates a GraphicsObject per name; meaning all names should be unique
	//Incase it already exists; it will use the existing one
	template<typename T>
	class GraphicsObjectRef {

		T *ptr{};

	public:

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
			ignis::Graphics &g, const String &name, const typename T::Info &info
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

	//Managed resources

	using CommandListRef = GraphicsObjectRef<ignis::CommandList>;
	using SwapchainRef = GraphicsObjectRef<ignis::Swapchain>;
	using FramebufferRef = GraphicsObjectRef<ignis::Framebuffer>;
	using PrimitiveBufferRef = GraphicsObjectRef<ignis::PrimitiveBuffer>;
	using GPUBufferRef = GraphicsObjectRef<ignis::GPUBuffer>;
	using ShaderBufferRef = GraphicsObjectRef<ignis::ShaderBuffer>;
	using PipelineRef = GraphicsObjectRef<ignis::Pipeline>;
	using PipelineLayoutRef = GraphicsObjectRef<ignis::PipelineLayout>;
	using TextureRef = GraphicsObjectRef<ignis::Texture>;
	using SamplerRef = GraphicsObjectRef<ignis::Sampler>;
	using DescriptorsRef = GraphicsObjectRef<ignis::Descriptors>;
	using UploadBufferRef = GraphicsObjectRef<ignis::UploadBuffer>;

}

namespace std {

	template<typename T>
	struct hash<igx::GraphicsObjectRef<T>> {

		inline usz operator()(const igx::GraphicsObjectRef<T> &t) const {
			return usz(t.get());
		}

	};

}