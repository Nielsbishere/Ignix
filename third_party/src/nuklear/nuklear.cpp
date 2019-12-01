#define NK_IMPLEMENTATION
#include "system/system.hpp"
#include "system/log.hpp"
#include "nuklear/nuklear.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/allocator.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "utils/hash.hpp"
#include "system/local_file_system.hpp"
#include "input/mouse.hpp"
#include "input/keyboard.hpp"

//A test to see how easy NK is to work with

using namespace oic;
using namespace igx;

//Wrapper for our nk allocator

HashMap<void*, usz> sizes;

void *nkAlloc(nk_handle, void*, nk_size count) {
	u8 *allocation = oic::System::allocator()->allocArray(count);
	sizes[allocation] = count;
	return allocation;
}

void nkFree(nk_handle, u8 *old) {

	if (!old) return;

	auto it = sizes.find(old);

	if (it == sizes.end())
		oic::System::log()->fatal("Couldn't nkFree");

	oic::System::allocator()->freeArray(old, it->second);
	sizes.erase(it);
}

oicExposedEnum(
	NMouseButton, int,
	BUTTON_LEFT = 0,
	BUTTON_MIDDLE = 1,
	BUTTON_RIGHT = 2
)

oicExposedEnum(
	NKey, int,
	KEY_SHIFT = NK_KEY_SHIFT,
	KEY_CTRL = NK_KEY_CTRL,
	KEY_DELETE = NK_KEY_DEL,
	KEY_ENTER = NK_KEY_ENTER,
	KEY_TAB = NK_KEY_TAB,
	KEY_BACKSPACE = NK_KEY_BACKSPACE,
	KEY_UP = NK_KEY_UP,
	KEY_DOWN = NK_KEY_DOWN,
	KEY_LEFT = NK_KEY_LEFT,
	KEY_RIGHT = NK_KEY_RIGHT
);

//Missing:
//NK_KEY_COPY,
//NK_KEY_CUT,
//NK_KEY_PASTE,
///* Shortcuts: text field */
//NK_KEY_TEXT_INSERT_MODE,
//NK_KEY_TEXT_REPLACE_MODE,
//NK_KEY_TEXT_RESET_MODE,
//NK_KEY_TEXT_LINE_START,
//NK_KEY_TEXT_LINE_END,
//NK_KEY_TEXT_START,
//NK_KEY_TEXT_END,
//NK_KEY_TEXT_UNDO,
//NK_KEY_TEXT_REDO,
//NK_KEY_TEXT_SELECT_ALL,
//NK_KEY_TEXT_WORD_LEFT,
//NK_KEY_TEXT_WORD_RIGHT,
///* Shortcuts: scrollbar */
//NK_KEY_SCROLL_START,
//NK_KEY_SCROLL_END,
//NK_KEY_SCROLL_DOWN,
//NK_KEY_SCROLL_UP,

//viewport interface

struct NKViewportInterface : public ViewportInterface {

	static constexpr usz MAX_MEMORY = 1_MiB;

	//Our app data

	Graphics &g;

	GPUBuffer ibo, vbo;
	Texture textureAtlas;
	PrimitiveBuffer primitiveBuffer;
	CommandList commands;
	Sampler sampler;

	GPUBuffer res;
	Swapchain swapchain;
	Pipeline pipeline;
	Framebuffer target;
	Descriptors descriptors;

	//Constants

	BufferAttributes vertexLayout = { GPUFormat::RG32f, GPUFormat::RG32f, GPUFormat::RGBA8 };

	PipelineLayout pipelineLayout = {
		RegisterLayout(NAME("Pipeline layout"), 0, SamplerType::SAMPLER_2D, 0, ShaderAccess::FRAGMENT),
		RegisterLayout(NAME("Res buffer"), 1, GPUBufferType::UNIFORM, 0, ShaderAccess::VERTEX, 8 /* res buffer size */)
	};

	u32 msaa = 8;
	bool refresh{};

	//nk init data

	Buffer commandList;
	nk_context *ctx;
	nk_font *font;
	nk_allocator allocator{};

	nk_draw_null_texture atlasTexture;
	Buffer previous;

	//

	NKViewportInterface(Graphics &g) : g(g) {

		//Allocate

		ctx = oic::System::allocator()->alloc<nk_context>();
		commandList.resize(MAX_MEMORY);

		commands = {
			g, NAME("Command list"),
			CommandList::Info(1_MiB)
		};

		res = {
			g, NAME("Resolution buffer"),
			GPUBuffer::Info(8, GPUBufferType::UNIFORM, GPUMemoryUsage::CPU_WRITE)
		};

		allocator.alloc = &nkAlloc;
		allocator.free = (nk_plugin_free)&nkFree;

		//Null texture (white)

		Buffer vertShader, fragShader;
		oicAssert("Couldn't find pass through vertex shader", oic::System::files()->read("./shaders/pass_through.vert.spv", vertShader));
		oicAssert("Couldn't find pass through fragment shader", oic::System::files()->read("./shaders/pass_through.frag.spv", fragShader));

		pipeline = {
			g, NAME("Test pipeline"),
			Pipeline::Info(
				PipelineFlag::OPTIMIZE,
				{ vertexLayout },
				{
					{ ShaderStage::VERTEX, vertShader },
					{ ShaderStage::FRAGMENT, fragShader }
				},
				pipelineLayout,
				PipelineMSAA(msaa, .2f),
				Rasterizer(),
				BlendState::alphaBlend()
			)
		};

		sampler = {
			g, NAME("Test sampler"),
			Sampler::Info()
		};

		//Init font and nk

		struct nk_font_atlas atlas {};
		nk_font_atlas_init(&atlas, &allocator);

		nk_font_atlas_begin(&atlas);

		font = nk_font_atlas_add_default(&atlas, 16 /* TODO */, nullptr);

		int width{}, height{};
		u8 *data = (u8*) nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_ALPHA8);

		Texture::Info info(
			Vec2u { u32(width), u32(height) }, GPUFormat::R8, GPUMemoryUsage::LOCAL, 1, 1
		);

		info.init({ Buffer(data, data + usz(width) * height) });

		textureAtlas = {
			g, NAME("Atlas texture"), info
		};


		DescriptorsSubresources resources;
		resources[0] = { sampler, textureAtlas };
		resources[1] = { res, 0 };

		descriptors = {
			g, NAME("Atlas descriptor"),
			Descriptors::Info(pipelineLayout, resources)
		};

		nk_font_atlas_end(&atlas, nk_handle_ptr(textureAtlas.get()), &atlasTexture);

		nk_init_fixed(ctx, commandList.data(), MAX_MEMORY, &font->handle);

		target = {
			g, NAME("Framebuffer output"),
			Framebuffer::Info({ GPUFormat::RGBA8 }, DepthFormat::NONE, false, msaa)
		};

		g.pause();
	}

	~NKViewportInterface() {
		oic::System::allocator()->free(ctx);
	}

	void init(ViewportInfo *vp) final override {
		swapchain = { g, NAME("Swapchain"), Swapchain::Info{vp, false, DepthFormat::NONE } };
	}

	void resize(const ViewportInfo *, const Vec2u &size) final override {

		swapchain->onResize(size);
		target->onResize(size);

		memcpy(res->getBuffer(), size.data(), sizeof(size));
		res->flush(0, 8);

		refresh = true;
	}

	void release(const ViewportInfo*) final override { }

	void onInputActivate(const ViewportInfo*, const InputDevice *dvc, InputHandle ih) {
		oic::System::log()->debug("Pressed ", dvc->nameByHandle(ih).c_str());
	}

	void onInputDeactivate(const ViewportInfo*, const InputDevice *dvc, InputHandle ih) {
		oic::System::log()->debug("Released ", dvc->nameByHandle(ih).c_str());
	}

	void update(const ViewportInfo *vi, f64) final override {

		//Receive events

		nk_input_begin(ctx);

		//Could potentially use a callback system for efficiency TODO:

		bool processedMouse{};

		for(auto *dvc : vi->devices)
			if (dvc->getType() == InputDevice::Type::KEYBOARD) {

				//Only loop through nuklear keys

				for (usz i = 0; i < NKey::count; ++i) {

					//Get our key

					String name = NKey::nameById(i);
					usz keyId = Key::idByName(name);

					if (keyId == Key::count) continue;

					//Send to NK

					auto state = dvc->getState(InputHandle(keyId));

					if (state == InputDevice::PRESSED)
						nk_input_key(ctx, nk_keys(NKey::values[i]), 1);

					else if (state == InputDevice::RELEASED)
						nk_input_key(ctx, nk_keys(NKey::values[i]), 0);

				}

			} else if(dvc->getType() == InputDevice::Type::MOUSE) {

				if (processedMouse) continue;

				f64 x = dvc->getCurrentAxis(MouseAxis::AXIS_X);
				f64 y = dvc->getCurrentAxis(MouseAxis::AXIS_Y);
				f64 px = dvc->getPreviousAxis(MouseAxis::AXIS_X);
				f64 py = dvc->getPreviousAxis(MouseAxis::AXIS_Y);

				if (px == x && py == y) continue;


				//Only loop through nuklear keys

				for (usz i = 0; i < NMouseButton::count; ++i) {

					//Get our key

					String name = NMouseButton::nameById(i);
					usz keyId = MouseButton::idByName(name);

					if (keyId == MouseButton::count) continue;

					nk_input_button(ctx, nk_buttons(i), int(x), int(y), int(dvc->getCurrentState(ButtonHandle(keyId))));
				}

				nk_input_motion(ctx, int(x), int(y));
				processedMouse = true;
			}

		nk_input_end(ctx);

	}

	void render(const ViewportInfo*) final override  {

		//Do render

		enum { EASY, NORMAL, HARD };
		static int op = EASY;
		static float value = 0.6f;
		static int i =  20;

		if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 220),
					 NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
			// fixed widget pixel width
			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "Button")) {
				// event handling
			}
			// fixed widget window ratio width
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "Easy", op == EASY)) op = EASY;
			if (nk_option_label(ctx, "Normal", op == NORMAL)) op = NORMAL;
			if (nk_option_label(ctx, "Hard", op == HARD)) op = HARD;

			// custom widget pixel width
			nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
			{
				nk_layout_row_push(ctx, 50);
				nk_label(ctx, "Volume:", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 110);
				nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
			}
			nk_layout_row_end(ctx);
		}
		nk_end(ctx);

		if (!refresh && previous.data())
			refresh = previous.size() != ctx->memory.needed || memcmp(previous.data(), ctx->memory.memory.ptr, previous.size());

		//Convert to draw data

		if (refresh) {

			refresh = false;

			static const struct nk_draw_vertex_layout_element vertLayout[] = {
				{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT,  vertexLayout[0].offset },
				{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, vertexLayout[1].offset },
				{ NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, vertexLayout[2].offset },
				{ NK_VERTEX_LAYOUT_END }
			};

			struct nk_convert_config cfg = {};
			cfg.shape_AA = NK_ANTI_ALIASING_ON;
			cfg.line_AA = NK_ANTI_ALIASING_ON;
			cfg.vertex_layout = vertLayout;
			cfg.vertex_size = vertexLayout.getStride();
			cfg.vertex_alignment = 4 /* Detect? */;
			cfg.circle_segment_count = 22;
			cfg.curve_segment_count = 22;
			cfg.arc_segment_count = 22;
			cfg.global_alpha = 1.0f;
			cfg.null = atlasTexture;

			nk_buffer cmds, verts, idx;

			nk_buffer_init(&cmds, &allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
			nk_buffer_init(&verts, &allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
			nk_buffer_init(&idx, &allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
			nk_convert(ctx, &cmds, &verts, &idx, &cfg);

			vbo.release();
			ibo.release();
			primitiveBuffer.release();

			auto vboStart = (u8 *)verts.memory.ptr;
			auto iboStart = (u8 *)idx.memory.ptr;

			vbo = {
				g, NAME("NK VBO"),
				GPUBuffer::Info(Buffer(vboStart, vboStart + verts.needed), GPUBufferType::VERTEX, GPUMemoryUsage::LOCAL)
			};

			ibo = {
				g, NAME("NK IBO"),
				GPUBuffer::Info(Buffer(iboStart, iboStart + idx.needed), GPUBufferType::INDEX, GPUMemoryUsage::LOCAL)
			};

			primitiveBuffer = {
				g, NAME("Primitive buffer"),
				PrimitiveBuffer::Info(
					BufferLayout(vbo, vertexLayout),
					BufferLayout(ibo, BufferAttributes(GPUFormat::R16u))
				)
			};

			commands->clear();
			commands->add(
				BindPipeline(pipeline),
				SetClearColor(Vec4f { 0, 0.5, 1, 1 }),
				BeginFramebuffer(target),
				SetViewportAndScissor(),
				ClearFramebuffer(target),
				BindPrimitiveBuffer(primitiveBuffer),
				BindDescriptors(descriptors)
			);

			const nk_draw_command *cmd {};
			nk_draw_index offset {};

			nk_draw_foreach(cmd, ctx, &cmds) {

				if (!cmd->elem_count) continue;

				Texture t = Texture::Ptr(cmd->texture.ptr);
				auto r = cmd->clip_rect;

				commands->add(
					r.w == 16384 ? SetScissor() : SetScissor({ u32(r.w), u32(r.h) }, { i32(r.x), i32(r.y) }),
					DrawInstanced::indexed(cmd->elem_count, 1, offset)
				);

				offset += u16(cmd->elem_count);
			}

			commands->add(
				EndFramebuffer()
			);

			nk_buffer_free(&cmds);
			nk_buffer_free(&verts);
			nk_buffer_free(&idx);

			g.present(target, swapchain, commands);

			u8 *prev = (u8*)ctx->memory.memory.ptr;

			previous = Buffer(prev, prev + ctx->memory.needed);

		} else
			g.present(target, swapchain);

		//Reset

		nk_clear(ctx);
	}

};

int main() {

	Graphics g;
	NKViewportInterface nkViewportInterface{ g };

	System::viewportManager()->create(
		ViewportInfo("NK test", {}, {}, 0, &nkViewportInterface, ViewportInfo::HANDLE_INPUT)
	);

	while (System::viewportManager()->size())
		System::wait(250_ms);

	return 0;
}