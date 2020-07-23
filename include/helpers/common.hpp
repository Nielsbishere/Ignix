#pragma once

//This header is made so it is easier to work with Ignis
//without having to include everything yourself
//it only provides access to high-level structures

#include "graphics/graphics.hpp"
#include "graphics/memory/swapchain.hpp"
#include "graphics/memory/framebuffer.hpp"
#include "graphics/memory/primitive_buffer.hpp"
#include "graphics/memory/shader_buffer.hpp"
#include "graphics/memory/upload_buffer.hpp"
#include "graphics/memory/texture.hpp"
#include "graphics/memory/render_texture.hpp"
#include "graphics/memory/depth_texture.hpp"
#include "graphics/shader/sampler.hpp"
#include "graphics/shader/pipeline.hpp"
#include "graphics/shader/descriptors.hpp"
#include "graphics/command/commands.hpp"

namespace igx {
	using namespace ignis::cmd;
	using namespace ignis;
}