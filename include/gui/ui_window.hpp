#pragma once
#include "types/vec.hpp"

namespace igx {

	class UIWindow {

	public:

		enum Flags : u32 {

			//Visual modifiers

			HAS_TITLE			= 1 << 0,
			BORDER				= 1 << 1,
			VISIBLE				= 1 << 2,
			SCROLL_AUTO_HIDE	= 1 << 3,

			//Functional modifiers

			INPUT				= 1 << 4,
			SCROLL				= 1 << 5,
			MOVE				= 1 << 6,
			SCALE				= 1 << 7,
			CLOSE				= 1 << 8,
			MINIMIZE			= 1 << 9,

			//Constraints

			HAS_MIN_X			= 1 << 10,
			HAS_MIN_Y			= 1 << 11,
			HAS_MAX_X			= 1 << 12,
			HAS_MAX_Y			= 1 << 13,

			//Combinations of flags

			NONE				= 0,
			STATIC				= INPUT | HAS_TITLE | BORDER | VISIBLE,
			STATIONARY			= STATIC | MINIMIZE | CLOSE,
			DEFAULT				= MOVE | SCALE | STATIONARY
		};

	private:

		Vec2f32 pos, dim;
		Vec2f32 min, max;

		String name;

		u32 id;

		Flags flags;

	public:

		//Create a window at a pixel position
		UIWindow(
			const String &name, const u32 id, const Vec2f32 &pos,
			const Vec2f32 &dim, Flags flags = DEFAULT
		);

		//Create a window at a pixel position with a minimum size
		UIWindow(
			const String &name, const u32 id, const Vec2f32 &pos, const Vec2f32 &dim,
			const Vec2f32 &min, Flags flags = DEFAULT
		);

		//Create a window at a pixel position with a minimum and maximum size
		UIWindow(
			const String &name, const u32 id, const Vec2f32 &pos, const Vec2f32 &dim, 
			const Vec2f32 &min, const Vec2f32 &max, Flags flags = DEFAULT
		);

		//Setters

		inline void hide();
		inline void show();

		inline void setVisible(bool b);

		inline void setMinX(f32 val);
		inline void setMinY(f32 val);
		inline void setMin(const Vec2f32 &val);

		inline void setMaxX(f32 val);
		inline void setMaxY(f32 val);
		inline void setMax(const Vec2f32 &val);

		inline void setBounds(const Vec2f32 &min, const Vec2f32 &max);

		void updateLocation(const Vec2f32 &po, const Vec2f32 &di) { pos = po; dim = di; }

		//Getters

		inline Vec2f32 getMin() const { return min; }
		inline Vec2f32 getMax() const { return max; }

		inline Vec2f32 getPos() const { return pos; }
		inline Vec2f32 getDim() const { return dim; }

		inline String getTitle() const { return name; }
		inline Flags getFlags() const { return flags; }
		inline u32 getId() const { return id; }

		inline bool visible() const { return flags & VISIBLE; }

		//Helpers

		Vec2f32 clampBounds(Vec2f32 bounds) const;

	private:

		template<Flags mask, typename T>
		inline void set(T &t, T val);

	};

	//Implementations

	inline void UIWindow::hide() { flags = Flags(flags & ~VISIBLE); }
	inline void UIWindow::show() { flags = Flags(flags | VISIBLE); }

	inline void UIWindow::setVisible(bool b) {
		if (b) show();
		else hide();
	}

	template<UIWindow::Flags mask, typename T>
	inline void UIWindow::set(T &t, T val) {

		if(val == 0)
			flags = Flags(flags & ~mask);
		else {
			flags = Flags(flags | mask);
			t = val;
		}
	}

	inline void UIWindow::setMinX(f32 val) { set<Flags::HAS_MIN_X>(min.x, val); }
	inline void UIWindow::setMinY(f32 val) { set<Flags::HAS_MIN_Y>(min.y, val); }
	inline void UIWindow::setMaxX(f32 val) { set<Flags::HAS_MAX_X>(max.x, val); }
	inline void UIWindow::setMaxY(f32 val) { set<Flags::HAS_MAX_Y>(max.y, val); }

	inline void UIWindow::setMin(const Vec2f32 &val) { setMinX(val.x); setMinY(val.y); }
	inline void UIWindow::setMax(const Vec2f32 &val) { setMaxX(val.x); setMaxY(val.y); }

	inline void UIWindow::setBounds(const Vec2f32 &mi, const Vec2f32 &ma) { 
		setMin(mi);
		setMax(ma);
	}

}