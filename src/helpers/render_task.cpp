#include "helpers/render_task.hpp"

namespace igx {

	void RenderTask::resize(const Vec2u32 &target) {
		needsCmdUpdate = true;
		currentSize = target;
	}

	void TextureRenderTask::resize(const Vec2u32 &size) {

		RenderTask::resize(size);

		for(usz i = 0; i < textures.size(); ++i) {

			auto &info = infos[i];
			auto &texture = textures[i];

			texture.release();

			info.dimensions = Vec3u16(u16(size.x), u16(size.y), 1);
			info.mipSizes = { info.dimensions };

			if (HasFlags(info.usage, GPUMemoryUsage::CPU_WRITE)) {

				info.pending = { { 0, info.dimensions } };
				info.markedPending = false;

				info.initData[0].resize(
					info.dimensions.prod<usz>() * FormatHelper::getSizeBytes(info.format)
				);
			}

			texture = { g, names[i], info };

		}
	}

	void GPUBufferRenderTask::resize(const Vec2u32 &size) {

		RenderTask::resize(size);
		buffer.release();

		info.size = stride * size.prod<usz>();

		if (HasFlags(info.usage, GPUMemoryUsage::CPU_WRITE)) {
			info.pending = { { 0, info.size } };
			info.markedPending = false;
			info.initData.resize(info.size);
		}

		buffer = { g, name, info };
	}

	void GraphicsRenderTask::resize(const Vec2u32 &size) {
		RenderTask::resize(size);
		framebuffer->onResize(size);
	}

	void RenderTasks::resize(const Vec2u32 &target) {
		for (RenderTask *task : renderTasks)
			task->resize(target);
	}

	void RenderTasks::prepareCommandList(CommandList *cl) {

		bool needsPrepare{};

		for(RenderTask *task : renderTasks)
			if (task->needsCommandUpdate()) {
				needsPrepare = true;
				break;
			}

		if (needsPrepare) {

			for (RenderTask *task : renderTasks)
				task->prepareCommandList(cl);
		}
	}

	void RenderTasks::update(f64 dt) {
		for (RenderTask *task : renderTasks)
			task->update(dt);
	}

	void RenderTasks::switchToScene(SceneGraph *sceneGraph) {
		for (RenderTask *task : renderTasks)
			task->switchToScene(sceneGraph);
	}

	void ParentTextureRenderTask::resize(const Vec2u32 &target) {
		TextureRenderTask::resize(target);
		tasks.resize(target);
	}

	bool ParentTextureRenderTask::needsCommandUpdate() const {

		if (TextureRenderTask::needsCommandUpdate())
			return true;

		for (RenderTask *task : tasks)
			if (task->needsCommandUpdate())
				return true;

		return false;
	}

	void ParentTextureRenderTask::prepareCommandList(CommandList *cl) {
		tasks.prepareCommandList(cl);
	}

	void ParentTextureRenderTask::update(f64 dt) {
		tasks.update(dt);
	}

	void ParentTextureRenderTask::switchToScene(SceneGraph *sceneGraph) {
		tasks.switchToScene(sceneGraph);
	}

}