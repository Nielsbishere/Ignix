#include "gui/gui.hpp"
#include "graphics/memory/render_texture.hpp"
#include "system/system.hpp"
#include "system/local_file_system.hpp"
#include "gui/window.hpp"

namespace igx::ui {

	GUI::GUI(Graphics &g, const SetClearColor &clearColor, usz commandListSize) :
		clearColor(clearColor), commandListSize(commandListSize),
		flags(Flags(Flags::OWNS_COMMAND_LIST | Flags::OWNS_FRAMEBUFFER))
	{
		init(g);
	}

	GUI::GUI(Graphics &g, const FramebufferRef &fb, usz commandListSize) :
		target(fb), commandListSize(commandListSize), flags(Flags::OWNS_COMMAND_LIST)
	{
		init(g);
	}

	GUI::GUI(Graphics &g, const CommandListRef &cl):
		commands(cl), flags(Flags::OWNS_FRAMEBUFFER)
	{
		init(g);
	}

	GUI::GUI(Graphics &g, const FramebufferRef &fb, const CommandListRef &cl) :
		target(fb), commands(cl), flags(Flags::NONE)
	{
		init(g);
	}

	void GUI::init(Graphics &g) {

		graphics = &g;

		guiDataBuffer = {
			g, NAME("GUI info buffer"),
			GPUBuffer::Info(
				sizeof(GUIInfo), GPUBufferType::UNIFORM, GPUMemoryUsage::CPU_ACCESS
			)
		};

		sampler = {
			g, NAME("GUI sampler"),
			Sampler::Info(SamplerMin::LINEAR, SamplerMag::LINEAR, SamplerMode::REPEAT, 1)
		};

		Buffer vertShader, fragShader;

		oicAssert("Couldn't find pass through vertex shader", oic::System::files()->read(FILE_PATH("~/igx/shaders/gui.vert.spv"), vertShader));
		oicAssert("Couldn't find pass through fragment shader", oic::System::files()->read(FILE_PATH("~/igx/shaders/gui.frag.spv"), fragShader));

		bool hasFb = !(flags & Flags::OWNS_FRAMEBUFFER);

		uiShader = {
			g, NAME("GUI pipeline"),
			Pipeline::Info(
				Pipeline::Flag::NONE,
				{ vertexLayout },
				{
					{ ShaderStage::VERTEX, { vertShader, "main" } },
					{ ShaderStage::FRAGMENT, { fragShader, "main" } }
				},
				pipelineLayout,
				MSAA(hasFb ? target->getInfo().samples : msaa, .2f),
				DepthStencil(),
				Rasterizer(CullMode::NONE),

				flags & Flags::DISABLE_SUBPIXEL_RENDERING ? BlendState::alphaBlend() : BlendState::subpixelAlphaBlend()
			)
		};

		if (flags & Flags::OWNS_COMMAND_LIST)
			commands = {
				g, NAME("GUI command list"),
				CommandList::Info(commandListSize)
			};

		if (flags & Flags::OWNS_FRAMEBUFFER)
			target = {
				g, NAME("GUI framebuffer"),
				Framebuffer::Info(
					{ GPUFormat::sRGBA8 },
					DepthFormat::NONE,
					false,
					msaa
				)
			};

		uploadBuffer = {
			g, NAME("GUI Upload buffer"),
			UploadBuffer::Info(64_KiB, 0, 1_MiB)
		};

		initData(g);
	}

	void GUI::resize(const Vec2u32 &size) {

		requestUpdate();

		//Resize targets
		
		if (flags & GUI::OWNS_FRAMEBUFFER)
			target->onResize(size);

		info.res = size.cast<Vec2i32>();
		needsBufferUpdate = true;

		//TODO: UIWindows
	}

	void GUI::requestUpdate() { 
		requestedUpdate = true;
	}

	void GUI::beginDraw() {

		if (flags & GUI::OWNS_COMMAND_LIST)
			commands->clear();

		commands->add(
			BindPipeline(uiShader),
			clearColor,
			BeginFramebuffer(target),
			BindDescriptors(descriptors),
			SetViewportAndScissor()
		);

		if (flags & GUI::OWNS_FRAMEBUFFER)
			commands->add(ClearFramebuffer());

	}

	void GUI::endDraw() {
		commands->add(EndFramebuffer());
	}

	void GUI::render(Graphics &g, const Vec2i32 &offset, const List<oic::Monitor> &mons) {

		bool changedMonitors{};

		if (monitors != mons) {

			monitors = mons;

			for(auto &mon : monitors)
				if (flags & Flags::DISABLE_SUBPIXEL_RENDERING) {
					mon.sampleR = {};
					mon.sampleG = {};
					mon.sampleB = {};
				}

			static constexpr usz maxMonitors = 256;

			if(guiMonitorBuffer.null())
				guiMonitorBuffer = {
					g, NAME("GUI monitor buffer"),
					GPUBuffer::Info(
						sizeof(mons[0]) * maxMonitors,
						GPUBufferType::STRUCTURED, GPUMemoryUsage::CPU_ACCESS
					)
				};

			usz dataSize = sizeof(mons[0]) * mons.size();

			if (std::memcmp(guiMonitorBuffer->getBuffer(), mons.data(), dataSize)) {
				std::memcpy(guiMonitorBuffer->getBuffer(), mons.data(), dataSize);
				guiMonitorBuffer->flush(0, dataSize);
			}

			descriptors->updateDescriptor(2, { guiMonitorBuffer, 0 });
			descriptors->flush({ { 2, 1 } });

			changedMonitors = true;
			requestedUpdate = true;
		}

		if (offset != info.pos) {
			info.pos = offset;
			needsBufferUpdate = true;
		}

		if (needsBufferUpdate) {

			std::memcpy(guiDataBuffer->getBuffer(), &info, sizeof(info));
			guiDataBuffer->flush(0, sizeof(info));

			needsBufferUpdate = false;
			requestedUpdate = true;
		}

		//Check if a graphical change occurred by checking via UI

		if (prepareDrawData() || requestedUpdate) {

			bakePrimitives(g);

			beginDraw();

			commands->add(
				FlushBuffer(guiDataBuffer, uploadBuffer),
				FlushBuffer(guiMonitorBuffer, uploadBuffer)
			);

			draw();
			endDraw();

			requestedUpdate = false;
		}

	}

}