#include <cstring>

//Nuklear Configuration

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_COMMAND_USERDATA
#define NK_ZERO_COMMAND_MEMORY

#define NK_PRIVATE
#define NK_INCLUDE_FONT_BAKING
#define NK_ENABLE_SUBPIXEL_API

#define NK_UINT_DRAW_INDEX
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#define NK_MEMCPY std::memcpy
#define NK_MEMSET std::memset

#include "system/system.hpp"
#include "system/log.hpp"

#define NK_ASSERT(...) oicAssert("Assert failed", __VA_ARGS__)
#define NK_ERROR(...) oic::System::log()->fatal(__VA_ARGS__)

#define NK_IMPLEMENTATION

#include "Nuklear/nuklear.h"
#include "system/allocator.hpp"
#include "system/local_file_system.hpp"
#include "types/enum.hpp"
#include "input/input_device.hpp"
#include "input/mouse.hpp"
#include "input/keyboard.hpp"
#include "gui/gui.hpp"
#include "gui/struct_inspector.hpp"
#include "gui/window.hpp"
#include "utils/math.hpp"
#include "utils/timer.hpp"

using namespace oic;
using namespace igx::ui;
using namespace igx;

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

//Nuklear mappings

oicExposedEnum(
	NMouseButton, int,
	Button_left,
	Button_middle,
	Button_right
)

oicExposedEnum(
	NKey, int,
	Key_shift = NK_KEY_SHIFT,
	Key_ctrl = NK_KEY_CTRL,
	Key_delete = NK_KEY_DEL,
	Key_enter = NK_KEY_ENTER,
	Key_tab = NK_KEY_TAB,
	Key_backspace = NK_KEY_BACKSPACE,
	Key_up = NK_KEY_UP,
	Key_down = NK_KEY_DOWN,
	Key_left = NK_KEY_LEFT,
	Key_right = NK_KEY_RIGHT
);

//GUI implementation for nuklear

namespace igx::ui {

	struct GUIData {

		static constexpr usz MAX_MEMORY = 32_KiB;

		GPUBufferRef ibo, vbo;
		TextureRef textureAtlas;
		PrimitiveBufferRef primitiveBuffer;

		nk_context *ctx;
		nk_font *font;

		nk_allocator allocator{};
		nk_buffer drawCommands{};

		nk_draw_null_texture nullTexture;

		usz usedPrevious{};
		Buffer current, previous;

		ns previousTime{};

		usz vboSize{}, iboSize{};

		bool init{};
	};

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
				RegisterLayout(NAME("Input texture"), 0, SamplerType::SAMPLER_2D, 0, ShaderAccess::FRAGMENT),
				RegisterLayout(NAME("GUI Info"), 1, GPUBufferType::UNIFORM, 0, ShaderAccess::VERTEX_FRAGMENT, sizeof(GUIInfo)),
				RegisterLayout(NAME("Monitor buffer"), 2, GPUBufferType::STRUCTURED, 0, ShaderAccess::FRAGMENT, sizeof(oic::Monitor)),
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

		Buffer font;
		oicAssert("GUI font required", oic::System::files()->read(VIRTUAL_FILE("~/igx/fonts/calibri.ttf"), font));

		data->font = nk_font_atlas_add_from_memory(&atlas, font.data(), font.size(), 13, nullptr);

		int width{}, height{};
		u8 *atlasData = (u8*) nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_ALPHA8);

		Texture::Info tinfo(
			Vec2u16{ u16(width), u16(height) }, GPUFormat::R8, GPUMemoryUsage::LOCAL, 1, 1
		);

		tinfo.init({ Buffer(atlasData, atlasData + usz(width) * height) });

		data->textureAtlas = {
			g, NAME("Atlas texture"), tinfo
		};

		Descriptors::Subresources resources;
		resources[0] = { sampler, data->textureAtlas, TextureType::TEXTURE_2D };
		resources[1] = { guiDataBuffer, 0 };

		descriptors = {
			g, NAME("Atlas descriptor"),
			Descriptors::Info(pipelineLayout, resources)
		};

		nk_font_atlas_end(&atlas, nk_handle_ptr(data->textureAtlas.get()), &data->nullTexture);

		info.whiteTexel = { data->nullTexture.uv.x, data->nullTexture.uv.y };
		needsBufferUpdate = true;

		//Init nk

		nk_init_fixed(data->ctx, data->current.data(), GUIData::MAX_MEMORY, &data->font->handle);

		/* TODO: Style
		auto &style = data->ctx->style;

		auto background = nk_rgb(0x30, 0x30, 0x30);
		auto active = nk_rgb(0x40, 0x40, 0x40);
		auto border = nk_rgb(0x28, 0x28, 0x28);
		auto text = nk_rgb(0x87, 0xCE, 0xEB);
		auto textHover = nk_rgb(0x2D, 0xAD, 0xE4);
		auto textActive = nk_rgb(0x13, 0xBF, 0xFF);

		style.text.color = text;

		style.window.background = background;
		style.window.border_color = border;

		style.button.border_color = border;
		style.button.text_active = textActive;
		style.button.text_hover = textHover;
		style.button.text_background = background;
		style.button.text_normal = text;
		style.button.active.data.color = active;

		style.contextual_button.border_color = border;
		style.contextual_button.text_active = textActive;
		style.contextual_button.text_hover = textHover;
		style.contextual_button.text_background = background;
		style.contextual_button.text_normal = text;
		style.contextual_button.active.data.color = active;

		style.menu_button.border_color = border;
		style.menu_button.text_active = textActive;
		style.menu_button.text_hover = textHover;
		style.menu_button.text_background = background;
		style.menu_button.text_normal = text;
		style.menu_button.active.data.color = active;

		struct nk_style_toggle option;
		struct nk_style_toggle checkbox;
		struct nk_style_selectable selectable;
		struct nk_style_slider slider;
		struct nk_style_progress progress;
		struct nk_style_property property;
		struct nk_style_edit edit;
		struct nk_style_chart chart;
		struct nk_style_scrollbar scrollh;
		struct nk_style_scrollbar scrollv;
		struct nk_style_tab tab;
		struct nk_style_combo combo;
		struct nk_style_window window;*/

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
		cfg.vertex_alignment = 4 /* Detect? */;
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

				auto vboInfo = GPUBuffer::Info(newSize, GPUBufferType::VERTEX, GPUMemoryUsage::CPU_ACCESS);
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

				auto iboInfo = GPUBuffer::Info(newSize, GPUBufferType::INDEX, GPUMemoryUsage::CPU_ACCESS);
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
						BufferLayout(data->ibo, BufferAttributes(0, GPUFormat::R32u))
					)
				};
			}

		} else {
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

		const nk_draw_command *cmd {};
		nk_draw_index offset {};

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

		if (dvc->getType() == InputDevice::Type::KEYBOARD) {

			String name = Key::nameById(ih);
			usz nkid = NKey::idByName(name);

			if (nkid != NKey::count) {
				nk_input_key(data->ctx, nk_keys(NKey::values[nkid]), int(isActive));
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

				if (axis == MouseAxis::Axis_wheel) {
					nk_input_scroll(data->ctx, nk_vec2(f32(dvc->getCurrentAxis(MouseAxis::Axis_wheel)), 0));
					return true;
				}

				else if (axis == MouseAxis::Axis_x || axis == MouseAxis::Axis_y) {
					nk_input_motion(data->ctx, int(x), int(y));
					return true;
				}

			}
		}

		return false;
	}

	//Nuklear renderer

	bool StructRenderer::doButton(const String &name) {
		nk_layout_row_dynamic(data->ctx, 30, 1);
		return nk_button_label(data->ctx, name.data());
	}

	void StructRenderer::doCheckbox(const String &name, bool &checkbox){

		nk_layout_row_dynamic(data->ctx, 30, 2);

		int active = checkbox;
		nk_checkbox_label(data->ctx, name.c_str(), &active);
		checkbox = active;
	}

	void StructRenderer::doStruct(const String &) {
		nk_layout_row_dynamic(data->ctx, 30, 2);
	}

	void StructRenderer::doInt(const String &name, isz byteSize, usz required, void *loc) {

		auto &temporaryData = map->operator[](loc);
		temporaryData.isStillPresent = true;

		//Setup data and filter out invalid data

		usz ubyteSize = std::abs(byteSize);
		auto &len = *(int*)temporaryData.data;

		if (
			temporaryData.heapData.size() != required + ubyteSize + 1 || 
			std::memcmp(temporaryData.heapData.data(), loc, ubyteSize)
		) {

			//Reset data

			temporaryData.heapData.resize(required + ubyteSize + 1);
			std::memcpy(temporaryData.heapData.data(), loc, ubyteSize);

			//Init string

			std::stringstream ss;

			switch (byteSize) {
				case 1: ss << (u16)*(u8*)loc;		break;
				case -1: ss << (i16)*(i8*)loc;		break;
				case 2: ss << *(u16*)loc;			break;
				case -2: ss << *(i16*)loc;			break;
				case 4: ss << *(u32*)loc;			break;
				case -4: ss << *(i32*)loc;			break;
				case 8: ss << *(u64*)loc;			break;
				case -8: ss << *(i64*)loc;
			}

			String str = ss.str();

			oicAssert("Unexpected int string out of bounds", str.size() <= required);

			//Copy and set null to the end of the string

			std::memcpy(temporaryData.heapData.data() + ubyteSize, str.data(), str.size());

			usz leftOver = required - str.size() + 1;

			if(leftOver)
				std::memset(temporaryData.heapData.data() + ubyteSize + str.size(), 0, leftOver);

			//Store length

			len = (int) str.size();
		}

		if (len < 0 || len > required)
			len = 0;

		//TODO: nkFilter = nk_filter_hex
		//TODO: Ignore - if unsigned, otherwise only allow at front
		//TODO: Callbacks for runes, so input can be used

		//Edit string

		nk_layout_row_dynamic(data->ctx, 30, 2);

		nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);

		nk_edit_string(
			data->ctx, NK_EDIT_FIELD, 
			(char*)(temporaryData.heapData.data() + ubyteSize), 
			&len, (int)required + 1, 
			nk_filter_decimal
		);

		//Output

		const c8 *start = (const c8*)(temporaryData.heapData.data() + ubyteSize);

		std::stringstream ss(String(start, start + len));
		
		switch (byteSize) {

			case 2: ss >> *(u16*)loc;			break;
			case -2: ss >> *(i16*)loc;			break;
			case 4: ss >> *(u32*)loc;			break;
			case -4: ss >> *(i32*)loc;			break;
			case 8: ss >> *(u64*)loc;			break;
			case -8: ss >> *(i64*)loc;			break;

			case 1: {
				u16 temp;
				if (ss >> temp) *(u8*)loc = u8(temp);
				break;
			}

			case -1: {
				i16 temp;
				if (ss >> temp) *(i8*)loc = i8(temp);
			}
		}

	}

	void *StructRenderer::beginList(const String &name, usz count, void *loc) {

		//TODO: Scrollbar

		nk_layout_row_dynamic(data->ctx, 400, 1);

		//Get nk_list_view from temporary data

		auto &temporaryData = map->operator[](loc);
		temporaryData.isStillPresent = true;
		
		if (temporaryData.heapData.size() != sizeof(nk_list_view))
			temporaryData.heapData.resize(sizeof(nk_list_view));

		auto v = (nk_list_view*)temporaryData.heapData.data();

		//(No need to clear since nk_list_view_begin sets it

		if (nk_list_view_begin(data->ctx, (nk_list_view*)v, name.c_str(), NK_WINDOW_BORDER, 25, (int)count)) {
			nk_layout_row_dynamic(data->ctx, 30, 2);
			return v;
		}

		return nullptr;
	}

	void StructRenderer::endList(void *ptr) {
		nk_list_view_end((nk_list_view*)ptr);
	}

	//TODO: size of combo box and of line should be changable in a layout or something

	void StructRenderer::doDropdown(const String&, usz &index, const List<const c8*> &names) {

		auto *ctx = data->ctx;

		int selected = int(index);

		if (selected < 0 || selected >= names.size()) 
			selected = 0;		//Avoid garbage data

		//TODO: Opening two after each other seems to infinite loop?

		nk_layout_row_dynamic(data->ctx, 30, 1);

		nk_combobox(ctx, names.data(), int(names.size()), &selected, 30, nk_vec2(150, 200));

		index = usz(selected);
	}

	void StructRenderer::doRadioButtons(const String&, usz &index, const List<const c8*> &names) {
		
		nk_layout_row_static(data->ctx, 30, 75, 2);

		for (usz i = 0; i < names.size(); ++i)
			if (nk_option_label(data->ctx, names[i], index == i))
				index = i;

	}

	usz StructRenderer::doFileSystem(oic::FileSystem *&fs, oic::FileHandle handle, const String &path, u32 &selected) {

		oicAssert("Local file system is not supported yet", path.empty());

		FileInfo info = fs->getVirtualFiles()[handle];

		if (nk_tree_element_push_id(data->ctx, info.isFolder() ? NK_TREE_NODE : NK_TREE_CHILD, info.path.c_str(), NK_MINIMIZED, (int*)&selected, (int)handle)) {

			if (info.isFolder())
				for (FileHandle i = info.folderHint; i != info.fileEnd; ++i)
					doFileSystem(fs, i, path, selected);

			nk_tree_pop(data->ctx);
		}

		return handle;
	}

	void StructRenderer::doFileSystem(const String &name, oic::FileSystem *&fs) {

		fs->lock();

		if (nk_tree_push_id(data->ctx, NK_TREE_TAB, name.c_str(), NK_MINIMIZED, 0)) {

			//Reserve temporary data

			auto &temporaryData = map->operator[](&fs);
			temporaryData.isStillPresent = true;

			//Go through virtual path

			doFileSystem(fs, 0, "", *(u32*)temporaryData.data);

			nk_tree_pop(data->ctx);
		}

		fs->unlock();
	}

	//Rendering windows

	void GUI::renderWindows(List<Window> &ws) {

		auto *ctx = data->ctx;

		List<usz> marked;
		marked.reserve(ws.size());

		usz i{};

		for (Window &w : ws) {

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

					nk_end(ctx);
					++i;
					continue;
				}

				w.setVisible(true);

				Vec2f32 dim = { wnd->bounds.w, wnd->bounds.h };

				w.updateLocation(Vec2f32(wnd->bounds.x, wnd->bounds.y), dim);

				//TODO: integrade layouts
				//TODO: labels for members

				w.render(data);
			}

			nk_end(ctx);

			++i;
		}

		for (auto rit = marked.rbegin(), rend = marked.rend(); rit != rend; ++rit)
			ws.erase(ws.begin() + *rit);

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

		renderWindows(windows);

		//Detect if different

		usz needed = ctx->memory.needed;

		bool refresh =
			data->previous.size() != needed || 
			memcmp(data->previous.data(), data->current.data(), needed);

		if (refresh) {

			if (data->usedPrevious < needed)
				data->previous.resize(needed);

			memcpy(data->previous.data(), data->current.data(), needed);
			data->usedPrevious = needed;
		}

		//Begin input

		nk_input_begin(ctx);

		return refresh;
	}

}