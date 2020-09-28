#pragma once
#include "common.hpp"

namespace igx {

	class SceneGraph;

	//Some basic classes for handling render tasks

	enum class RenderMode : u8 {
		LQ,
		MQ,
		HQ,
		UQ,
		LQ_VIDEO,
		MQ_VIDEO,
		HQ_VIDEO,
		UQ_VIDEO
	};

	class RenderTask {

	protected:

		Graphics &g;

	private:

		String name;

		Vec4f32 debugColor;

		Vec2u32 currentSize;
		bool needsCmdUpdate = true;

	public:

		RenderTask(Graphics &g, const String &name, const Vec4f32 &debugColor = Vec4f32(0, 1, 0, 1)): 
			g(g), name(name), debugColor(debugColor) {}

		virtual ~RenderTask() {}

		RenderTask(const RenderTask&) = delete;
		RenderTask(RenderTask&&) = delete;
		RenderTask &operator=(const RenderTask&) = delete;
		RenderTask &operator=(RenderTask&&) = delete;

		void endCommandList(CommandList *cl);
		void startCommandList(CommandList *cl);

		virtual void prepareCommandList(CommandList *cl) = 0;

		virtual void update(f64 dt) = 0;
		virtual void resize(const Vec2u32 &target);

		virtual void switchToScene(SceneGraph *sceneGraph) = 0;
		virtual void prepareMode(RenderMode) {}

		const Vec2u32 &size() const { return currentSize; }

		inline void markNeedCmdUpdate() { needsCmdUpdate = true; }

		virtual bool needsCommandUpdate() const { return needsCmdUpdate; }

	};

	class TextureRenderTask : public RenderTask {

		List<Texture::Info> infos;
		List<TextureRef> textures;
		List<String> names;

	public:

		TextureRenderTask(
			Graphics &g, const Texture::Info &info, const String &name, 
			const Vec4f32 &debugColor = Vec4f32(0, 1, 0, 1)
		) :
			RenderTask(g, name, debugColor), infos{ info }, names{ name }, textures(1)
		{
			oicAssert("Only supporting one mip", info.mips == 1);
		}

		template<typename ...args>
		TextureRenderTask(Graphics &g, const String &name, const Vec4f32 &debugColor, const List<String> &names, const Texture::Info &info, const args &...arg) :
			RenderTask(g, name, debugColor), infos{ info, arg... }, names(names), textures(1 + sizeof...(arg))
		{
			for(auto &inf : infos)
				oicAssert("Only supporting one mip", inf.mips == 1);
		}

		virtual void resize(const Vec2u32&) override;

		inline const Texture::Info &getInfo(usz i = 0) const { return infos[i]; }
		inline Texture *getTexture(u32 i = 0) const { return textures[i]; }
		inline const String &getName(u32 i = 0) const { return names[i]; }
	};

	class GraphicsRenderTask : public RenderTask {

		FramebufferRef framebuffer;

	public:

		GraphicsRenderTask(
			Graphics &g, const Framebuffer::Info &info, const String &name, 
			const Vec4f32 &debugColor = Vec4f32(0, 1, 0, 1)
		) :
			RenderTask(g, name, debugColor), framebuffer(g, name, info) {}

		virtual void resize(const Vec2u32&) override;

		inline const Framebuffer::Info &getInfo() const { return framebuffer->getInfo(); }
		inline Framebuffer *getFramebuffer() const { return framebuffer; }
	};

	class GPUBufferRenderTask : public RenderTask {

		GPUBuffer::Info info;
		GPUBufferRef buffer;
		usz stride;

		String name;

	public:

		GPUBufferRenderTask(
			Graphics &g, const GPUBuffer::Info &info, const String &name, 
			const Vec4f32 &debugColor = Vec4f32(0, 1, 0, 1)
		) :
			RenderTask(g, name, debugColor), stride(info.size), name(name), info(info) { }

		virtual void resize(const Vec2u32&) override;

		inline const GPUBuffer::Info &getInfo() const { return buffer->getInfo(); }
		inline GPUBuffer *getBuffer() const { return buffer; }
	};

	//A container for ensuring they get cleaned up correctly

	class RenderTasks {

		List<RenderTask*> renderTasks;

	public:

		RenderTasks() {}

		RenderTasks(const RenderTasks&) = delete;
		RenderTasks(RenderTasks&&) = delete;
		RenderTasks &operator=(const RenderTasks&) = delete;
		RenderTasks &operator=(RenderTasks&&) = delete;

		~RenderTasks() {

			for (RenderTask *renderTask : renderTasks)
				delete renderTask;

			renderTasks.clear();
		}

		inline auto begin() { return renderTasks.begin(); }
		inline auto end() { return renderTasks.begin(); }
		inline auto &operator[](usz i) { return renderTasks[i]; }

		inline auto begin() const { return renderTasks.begin(); }
		inline auto end() const { return renderTasks.begin(); }
		inline auto &operator[](usz i) const { return renderTasks[i]; }

		template<typename T, typename ...args>
		inline void add(T *t, args *...arg) {

			static_assert(std::is_base_of_v<RenderTask, T>, "RenderTasks::add(T*, ...) requires T to be a RenderTask");

			renderTasks.push_back(t);

			if constexpr (sizeof...(arg) > 0)
				add(arg...);
		}

		template<typename T>
		inline T *get(usz i) const {
			static_assert(std::is_base_of_v<RenderTask, T>, "RenderTasks::get<T>(i) requires T to be a RenderTask");
			return dynamic_cast<T*>(operator[](i));
		}

		void resize(const Vec2u32 &target);

		void prepareCommandList(CommandList *cl);
		void update(f64 dt);

		void switchToScene(SceneGraph *sceneGraph);
	};

	//A texture task with children

	class ParentTextureRenderTask : public TextureRenderTask {

	protected:

		RenderTasks tasks;

	public:

		ParentTextureRenderTask(Graphics &g, const Texture::Info &info, const String &name) :
			TextureRenderTask(g, info, name) {}

		template<typename ...args>
		ParentTextureRenderTask(
			Graphics &g, const String &name, const Vec4f32 &debugColor, 
			const List<String> &names, const Texture::Info &inf0, const args &...arg
		):
			TextureRenderTask(g, name, debugColor, names, inf0, arg...) {}

		void resize(const Vec2u32 &target) override;

		bool needsCommandUpdate() const;
		void prepareCommandList(CommandList *cl) override;

		void update(f64 dt) override;

		void switchToScene(SceneGraph *sceneGraph) override;
	};

}