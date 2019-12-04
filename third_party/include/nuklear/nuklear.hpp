//Configuration

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_COMMAND_USERDATA
#define NK_ZERO_COMMAND_MEMORY

//TODO: Abstract this in igxi::UI

#define NK_PRIVATE
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include <string>

#define NK_MEMCPY std::memcpy
#define NK_MEMSET std::memset

#include "../nuklear/nuklear.h"