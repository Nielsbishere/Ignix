#include "gui/ui_window.hpp"
#include "utils/math.hpp"

using namespace oic;

namespace igx {

	UIWindow::UIWindow(
		const String &name, const u32 id, const Vec2f32 &pos,
		const Vec2f32 &dim, Flags flags
	):
		name(name), pos(pos), dim(dim), id(id), flags(flags) {}

	UIWindow::UIWindow(
		const String &name, const u32 id, const Vec2f32 &pos, const Vec2f32 &dim,
		const Vec2f32 &min, Flags flags
	):
		name(name), pos(pos), dim(dim), id(id), flags(flags) { setMin(min); }

	UIWindow::UIWindow(
		const String &name, const u32 id, const Vec2f32 &pos, const Vec2f32 &dim, 
		const Vec2f32 &min, const Vec2f32 &max, Flags flags
	):
		name(name), pos(pos), dim(dim), id(id), flags(flags) { setBounds(min, max); }


	Vec2f32 UIWindow::clampBounds(Vec2f32 bounds) const {

		if (flags & HAS_MIN_X) bounds.x = Math::max(bounds.x, min.x);
		if (flags & HAS_MIN_Y) bounds.y = Math::max(bounds.y, min.y);

		if (flags & HAS_MAX_X) bounds.x = Math::min(bounds.x, max.x);
		if (flags & HAS_MAX_Y) bounds.y = Math::min(bounds.y, max.y);

		return bounds;
	}

}