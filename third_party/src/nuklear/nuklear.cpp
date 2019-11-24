#define NK_IMPLEMENTATION
#include "system/system.hpp"
#include "system/log.hpp"
#include "nuklear/nuklear.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/allocator.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "utils/hash.hpp"

//A test to see how easy NK is to work with

using namespace oic;
using namespace igx;

//Wrapper for our nk allocator

HashMap<void*, usz> sizes;

void *nkAlloc(nk_handle, void *old, nk_size count) {

	u8 *allocation = oic::System::allocator()->allocArray(count);

	sizes[allocation] = count;

	if(old)
		memcpy(allocation, old, count);

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

//viewport interface

struct NKViewportInterface : public ViewportInterface {

	static constexpr usz MAX_MEMORY = 1_MiB;

	//Our app data

	Graphics &g;

	Texture textureAtlas;
	Texture nullTexture;

	Swapchain swapchain;

	//nk init data

	Buffer commandList;
	nk_context *ctx;
	nk_font *font;
	nk_allocator allocator{};

	nk_draw_null_texture atlasTexture;

	//

	NKViewportInterface(Graphics &g) : g(g) {

		//Allocate

		ctx = oic::System::allocator()->alloc<nk_context>();
		commandList.resize(MAX_MEMORY);

		allocator.alloc = &nkAlloc;
		allocator.free = (nk_plugin_free)&nkFree;

		//Null texture (white)

		u8 nullData[1][1] = { { 0xFF } };

		nullTexture = {
			g, NAME("Test texture"),
			Texture::Info(
				List<Grid2D<u8>>{
					{ nullData }
				},
				GPUFormat::R8, GPUMemoryUsage::LOCAL, 1
			)
		};

		//Init font and nk

		struct nk_font_atlas atlas {};
		nk_font_atlas_init(&atlas, &allocator);

		nk_font_atlas_begin(&atlas);

		font = nk_font_atlas_add_default(&atlas, 16 /* TODO */, nullptr);

		atlasTexture = {
			nk_handle_ptr(nullTexture.get()),
			{ 0.5f, 0.5f }
		};

		int width{}, height{};
		u8 *data = (u8*) nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_ALPHA8);

		Texture::Info info(
			Vec2u { u32(width), u32(height) }, GPUFormat::R8, GPUMemoryUsage::LOCAL, 1, 1
		);

		info.init({ Buffer(data, data + width * height) });

		textureAtlas = {
			g, NAME("Atlas texture"), info
		};

		nk_font_atlas_end(&atlas, nk_handle_ptr(textureAtlas.get()), &atlasTexture);

		nk_init_fixed(ctx, commandList.data(), MAX_MEMORY, &font->handle);

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
	}

	void release(const ViewportInfo*) final override { }

	void render(const ViewportInfo*) final override  {

		//Input

		nk_input_begin(ctx);
		//TODO: nk events
		nk_input_end(ctx);

		//Do render

		enum {EASY, HARD};
		static int op = EASY;
		static float value = 0.6f;
		static int i =  20;

		if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 220),
					 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
			// fixed widget pixel width
			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "Button")) {
				// event handling
			}
			// fixed widget window ratio width
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "Easy", op == EASY)) op = EASY;
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

		//Convert to ignis draw commands

		const struct nk_command *cmd = 0;
		nk_foreach(cmd, ctx) {

			int debug = 0;
			debug;

				//NK_COMMAND_NOP,
				//NK_COMMAND_SCISSOR,
				//NK_COMMAND_LINE,
				//NK_COMMAND_CURVE,
				//NK_COMMAND_RECT,
				//NK_COMMAND_RECT_FILLED,
				//NK_COMMAND_RECT_MULTI_COLOR,
				//NK_COMMAND_CIRCLE,
				//NK_COMMAND_CIRCLE_FILLED,
				//NK_COMMAND_ARC,
				//NK_COMMAND_ARC_FILLED,
				//NK_COMMAND_TRIANGLE,
				//NK_COMMAND_TRIANGLE_FILLED,
				//NK_COMMAND_POLYGON,
				//NK_COMMAND_POLYGON_FILLED,
				//NK_COMMAND_POLYLINE,
				//NK_COMMAND_TEXT,
				//NK_COMMAND_IMAGE,
				//NK_COMMAND_CUSTOM

				/*case NK_COMMAND_LINE:
					your_draw_line_function(...);
					break;

				case NK_COMMAND_RECT
					your_draw_rect_function(...);
					break;*/
		}

		g.present(nullptr, swapchain);

		//Reset

		nk_clear(ctx);
	}

};

int main() {

	Graphics g;
	NKViewportInterface nkViewportInterface{ g };

	System::viewportManager()->create(
		ViewportInfo("NK test", {}, {}, 0, &nkViewportInterface)
	);

	while (System::viewportManager()->size())
		System::wait(250_ms);

	return 0;
}