#include "gui/window.hpp"
#include "utils/math.hpp"

using namespace oic;

namespace igx::ui {

	Window::Window(
		const String &name, const u32 id, const Vec2f32 &pos,
		const Vec2f32 &dim, WindowInterface *interface, Flags flags
	):
		pos(pos), dim(dim), name(name), id(id), flags(flags), interface(interface) {}

}