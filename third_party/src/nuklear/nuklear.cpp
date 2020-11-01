#include "nuklear/nuklear.inc.hpp"
#include "nuklear/nk_struct_renderer.inc.hpp"
#include "system/local_file_system.hpp"
#include "system/allocator.hpp"
#include "input/input_device.hpp"
#include "input/mouse.hpp"
#include "input/keyboard.hpp"
#include "gui/gui.hpp"
#include "gui/window.hpp"
#include "utils/math.hpp"
#include "utils/timer.hpp"

//Nuklear allocator

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

//GUI implementation for nuklear

namespace igx::ui {

	using namespace oic;

	//Handling creation and deletion

	GUI::~GUI() {

		if (data) {
			nk_buffer_free(&data->drawCommands);
			oic::System::allocator()->free(data->ctx);
			oic::System::allocator()->free(data);
		}
	}
	
	void GUI::initData(Graphics &g) {

		pipelineLayout = {
			g, NAME("UI Pipeline layout"),
			PipelineLayout::Info{
				RegisterLayout(NAME("Input texture"), 0, SamplerType::SAMPLER_2D, 0, 0, ShaderAccess::FRAGMENT),
				RegisterLayout(NAME("GUI Info"), 1, GPUBufferType::UNIFORM, 0, 0, ShaderAccess::VERTEX_FRAGMENT, sizeof(GUIInfo)),
				RegisterLayout(NAME("Monitor buffer"), 2, GPUBufferType::STRUCTURED, 0, 0, ShaderAccess::FRAGMENT, sizeof(oic::Monitor)),
			}
		};

		//Allocate memory

		data = oic::System::allocator()->alloc<GUIData>();

		data->ctx = oic::System::allocator()->alloc<nk_context>();
		data->current.resize(GUIData::MAX_MEMORY);

		data->allocator.alloc = &nkAlloc;
		data->allocator.free = (nk_plugin_free)&nkFree;

		//Init font

		struct nk_font_atlas atlas{};
		nk_font_atlas_init(&atlas, &data->allocator);

		nk_font_atlas_begin(&atlas);

		//TODO: Get font from oic::System
		//		Saving memory (maybe allowing us to be < 0.5 MiB)

		Buffer font;
		oicAssert("GUI font required", oic::System::files()->read(VIRTUAL_FILE("igx/fonts/calibri.ttf"), font));

		data->font = nk_font_atlas_add_from_memory(&atlas, font.data(), font.size(), 13, nullptr);

		int width{}, height{};
		u8 *atlasData = (u8*) nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_ALPHA8);

		Texture::Info tinfo(
			Vec2u16{ u16(width), u16(height) }, GPUFormat::r8, GPUMemoryUsage::LOCAL, 1, 1
		);

		tinfo.init({ Buffer(atlasData, atlasData + usz(width) * height) });

		data->textureAtlas = {
			g, NAME("Atlas texture"), tinfo
		};

		Descriptors::Subresources resources;
		resources[0] = GPUSubresource(sampler, data->textureAtlas, TextureType::TEXTURE_2D);
		resources[1] = GPUSubresource(guiDataBuffer, GPUBufferType::UNIFORM);

		descriptors = {
			g, NAME("Atlas descriptor"),
			Descriptors::Info(pipelineLayout, 0, resources)
		};

		nk_font_atlas_end(&atlas, nk_handle_ptr(data->textureAtlas.get()), &data->nullTexture);

		info.whiteTexel = { data->nullTexture.uv.x, data->nullTexture.uv.y };
		needsBufferUpdate = true;

		//Init nk

		nk_init_fixed(data->ctx, data->current.data(), GUIData::MAX_MEMORY, &data->font->handle);

		//TODO: Style
	}

	//Converting to vertex buffer

	void GUI::bakePrimitives(Graphics &g) {

		auto *ctx = data->ctx;

		if (data->drawCommands.allocated)
			nk_buffer_free(&data->drawCommands);

		static const struct nk_draw_vertex_layout_element vertLayout[] = {
			{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT, vertexLayout[0].offset },
			{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, vertexLayout[1].offset },
			{ NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, vertexLayout[2].offset },
			{ NK_VERTEX_LAYOUT_END }
		};

		struct nk_convert_config cfg = {};
		cfg.shape_AA = NK_ANTI_ALIASING_ON;
		cfg.line_AA = NK_ANTI_ALIASING_ON;
		cfg.vertex_layout = vertLayout;
		cfg.vertex_size = vertexLayout.getStride();
		cfg.vertex_alignment = 4;
		cfg.circle_segment_count = 22;
		cfg.curve_segment_count = 22;
		cfg.arc_segment_count = 22;
		cfg.global_alpha = 1.0f;
		cfg.null = data->nullTexture;

		nk_buffer verts, idx;

		nk_buffer_init(&data->drawCommands, &data->allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
		nk_buffer_init(&verts, &data->allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
		nk_buffer_init(&idx, &data->allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
		nk_convert(ctx, &data->drawCommands, &verts, &idx, &cfg);

		if (verts.needed) {

			data->vboSize = verts.needed;
			data->iboSize = idx.needed;

			bool recreatePbuffer = false;

			//Resize if not enough vertices or copy into buffer

			if (!data->vbo.exists() || verts.needed > data->vbo->size()) {

				recreatePbuffer = true;
				data->vbo.release();

				usz newSize = Math::max(verts.needed * 2, 1_MiB);

				usz stride = vertexLayout.getStride();

				if (newSize % stride)
					newSize = (newSize / stride + 1) * stride;

				auto vboInfo = GPUBuffer::Info(newSize, GPUBufferUsage::VERTEX, GPUMemoryUsage::CPU_WRITE);
				std::memcpy(vboInfo.initData.data(), verts.memory.ptr, verts.needed);

				data->vbo = {
					g, NAME("NK VBO"),
					vboInfo
				};
			}
			
			else if (std::memcmp(data->vbo->getBuffer(), verts.memory.ptr, verts.needed)) {
				std::memcpy(data->vbo->getBuffer(), verts.memory.ptr, verts.needed);
				data->vbo->flush(0, verts.needed);
			}

			//Resize if not enough indices or copy into buffer

			if (!data->ibo.exists() || idx.needed > data->ibo->size()) {

				recreatePbuffer = true;
				data->ibo.release();

				usz newSize = Math::max(idx.needed * 2, 1_MiB);

				static constexpr usz stride = 2;

				if (newSize % stride)
					newSize = (newSize / stride + 1) * stride;

				auto iboInfo = GPUBuffer::Info(newSize, GPUBufferUsage::INDEX, GPUMemoryUsage::CPU_WRITE);
				std::memcpy(iboInfo.initData.data(), idx.memory.ptr, idx.needed);

				data->ibo = {
					g, NAME("NK IBO"),
					iboInfo
				};

			}

			else if (std::memcmp(data->ibo->getBuffer(), idx.memory.ptr, idx.needed)) {
				std::memcpy(data->ibo->getBuffer(), idx.memory.ptr, idx.needed);
				data->ibo->flush(0, idx.needed);
			}

			//Recreate primitive buffer

			if (recreatePbuffer) {

				data->primitiveBuffer.release();

				data->primitiveBuffer = {
					g, NAME("Primitive buffer"),
					PrimitiveBuffer::Info(
						BufferLayout(data->vbo, vertexLayout),
						BufferLayout(data->ibo, BufferAttributes(0, GPUFormat::r32u))
					)
				};
			}
		}
		else {
			data->vbo.release();
			data->ibo.release();
			data->primitiveBuffer.release();
		}

		nk_buffer_free(&verts);
		nk_buffer_free(&idx);
	}

	//Draw commands

	void GUI::draw() {

		commands->add(
			FlushImage(data->textureAtlas, uploadBuffer)
		);

		if (data->primitiveBuffer)
			commands->add(
				BindPrimitiveBuffer(data->primitiveBuffer),
				FlushBuffer(data->vbo, uploadBuffer),
				FlushBuffer(data->ibo, uploadBuffer)
			);

		const nk_draw_command *cmd{};
		nk_draw_index offset{};

		nk_draw_foreach(cmd, data->ctx, &data->drawCommands) {

			if (!cmd->elem_count) continue;

			auto r = cmd->clip_rect;

			commands->add(

				r.w == 16384 ? SetScissor()
				: SetScissor(
					Vec2u32(u32(r.w), u32(r.h)), 
					Vec2i32(i32(r.x), i32(r.y))
				),

				DrawInstanced::indexed(cmd->elem_count, 1, offset)
			);

			offset += u16(cmd->elem_count);
		}
	}
	
	//Receive input

	bool GUI::onInputUpdate(const InputDevice *dvc, InputHandle ih, bool isActive) {

		bool hasFocus{};

		for (auto &win : windows)
			if (win.hasFocus()) {
				hasFocus = true;
				break;
			}

		if (dvc->getType() == InputDevice::Type::KEYBOARD) {

			if (ih >= Key::count || !hasFocus)
				return false;

			String name = Key::nameById(ih);
			usz nkid = NKey::idByName(name);

			if (nkid != NKey::count) {
				nk_input_key(data->ctx, nk_keys(NKey::values[nkid]), int(isActive));
				return true;
			}

			if(isActive)
				if (c32 character = ((Keyboard*)dvc)->getKey((ButtonHandle)ih)) {
					nk_input_glyph(data->ctx, *(nk_glyph*)&character);
					return true;
				}

		} else if (dvc->getType() == InputDevice::Type::MOUSE) {

			f64 x = dvc->getCurrentAxis(MouseAxis::Axis_x);
			f64 y = dvc->getCurrentAxis(MouseAxis::Axis_y);

			//Buttons
			if (ih < MouseButton::count) {

				String name = MouseButton::nameById(ih);
				usz nkid = NMouseButton::idByName(name);

				if (nkid != MouseButton::count) {
					nk_input_button(data->ctx, nk_buttons(NMouseButton::values[nkid]), int(x), int(y), int(isActive));
					return true;
				}

			}

			//Wheel or x/y
			else {

				usz axis = ih - MouseButton::count;

				if (axis == MouseAxis::Axis_x || axis == MouseAxis::Axis_y) {
					nk_input_motion(data->ctx, int(x), int(y));
					return true;
				}

				if (!hasFocus)
					return false;

				if (axis == MouseAxis::Axis_wheel_x) {
					nk_input_scroll(data->ctx, nk_vec2(f32(dvc->getCurrentAxis(MouseAxis::Axis_wheel_x)), 0));
					return true;
				}

				else if (axis == MouseAxis::Axis_wheel_y) {
					nk_input_scroll(data->ctx, nk_vec2(0, f32(dvc->getCurrentAxis(MouseAxis::Axis_wheel_y))));
					return true;
				}

			}
		}

		return false;
	}

	//Rendering windows

	void GUI::renderWindows() {

		auto *ctx = data->ctx;

		List<usz> marked;
		marked.reserve(windows.size());

		usz i{};

		for (Window &w : windows) {

			using Flags = Window::Flags;
			Flags flag = w.getFlags();
			nk_flags nkFlags{};

			if (!(flag & Flags::INPUT))			nkFlags |= NK_WINDOW_NO_INPUT;
			if (!(flag & Flags::SCROLL))		nkFlags |= NK_WINDOW_NO_SCROLLBAR;
			if (flag & Flags::MOVE)				nkFlags |= NK_WINDOW_MOVABLE;
			if (flag & Flags::SCALE)			nkFlags |= NK_WINDOW_SCALABLE;
			if (flag & Flags::CLOSE)			nkFlags |= NK_WINDOW_CLOSABLE;
			if (flag & Flags::MINIMIZE)			nkFlags |= NK_WINDOW_MINIMIZABLE;
			if (flag & Flags::HAS_TITLE)		nkFlags |= NK_WINDOW_TITLE;
			if (flag & Flags::BORDER)			nkFlags |= NK_WINDOW_BORDER;
			if (flag & Flags::SCROLL_AUTO_HIDE)	nkFlags |= NK_WINDOW_SCROLL_AUTO_HIDE;

			struct nk_rect rect = nk_rect(w.getPos().x, w.getPos().y, w.getDim().x, w.getDim().y);

			if (nk_window *wnd = nk_add_window(ctx, w.getId(), w.getTitle().c_str(), rect, nkFlags)) {

				if (!nk_window_has_contents(wnd)) {

					if (wnd->flags & NK_WINDOW_CLOSED)
						marked.insert(marked.begin(), i);
					else
						w.setVisible(false);

					w.setFocus(false);
					nk_end(ctx);
					++i;
					continue;
				}

				w.setVisible(true);

				Vec2f32 dim = { wnd->bounds.w, wnd->bounds.h };

				w.updateLocation(Vec2f32(wnd->bounds.x, wnd->bounds.y), dim);

				//TODO: integrade layouts
				//TODO: labels for all members

				w.render(data);

				w.setFocus(nk_item_is_any_active(ctx));
			}

			nk_end(ctx);

			++i;
		}

		for (auto rit = marked.rbegin(), rend = marked.rend(); rit != rend; ++rit)
			windows.erase(windows.begin() + *rit);

	}

	//Nuklear input loop

	bool GUI::prepareDrawData() {

		auto *ctx = data->ctx;

		//Clear previous and capture input

		nk_clear(ctx);
		nk_input_end(data->ctx);

		f32 delta = f32(Timer::now() - data->previousTime) / 1_s;

		if (!data->previousTime) delta = 0;

		ctx->delta_time_seconds = f32(delta);
		data->previousTime = Timer::now();

		//Do render

		renderWindows();

		//Detect if different

		usz needed = ctx->memory.needed;

		bool refresh =
			data->previous.size() != needed || 
			std::memcmp(data->previous.data(), data->current.data(), needed);

		if (refresh) {

			if (data->usedPrevious < needed)
				data->previous.resize(needed);

			std::memcpy(data->previous.data(), data->current.data(), needed);
			data->usedPrevious = needed;
		}

		//Begin input

		nk_input_begin(ctx);

		return refresh;
	}

}