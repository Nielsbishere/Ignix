#include "system/system.hpp"
#include "system/log.hpp"
#include "types/enum.hpp"
#include "helpers/common.hpp"

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

#define NK_ASSERT(...) oicAssert("Assert failed", __VA_ARGS__)
#define NK_ERROR(...) oic::System::log()->fatal(__VA_ARGS__)

#define NK_IMPLEMENTATION
#include "Nuklear/nuklear.h"

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

}