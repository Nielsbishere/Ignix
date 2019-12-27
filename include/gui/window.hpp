#pragma once
#include "types/vec.hpp"
#include "window_container.hpp"

namespace igx::ui {

	class Window /*: public ElementContainer*/ {

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

			//Combinations of flags

			NONE				= 0,
			STATIC				= INPUT | HAS_TITLE | BORDER | VISIBLE,
			STATIONARY			= STATIC | MINIMIZE | CLOSE,
			DEFAULT				= MOVE | SCALE | STATIONARY,
			NO_MENU				= INPUT | VISIBLE | MOVE | SCALE,
			STATIC_NO_MENU		= INPUT | VISIBLE
		};

	private:

		Vec2f32 pos, dim;

		String name;

		u32 id;

		Flags flags;

	public:

		//Create a window at a pixel position
		Window(
			const String &name, const u32 id, const Vec2f32 &pos,
			const Vec2f32 &dim, Flags flags = DEFAULT
		);

		//Setters

		inline void hide();
		inline void show();

		inline void setVisible(bool b);

		void updateLocation(const Vec2f32 &po, const Vec2f32 &di) { pos = po; dim = di; }

		//Getters

		inline Vec2f32 getPos() const { return pos; }
		inline Vec2f32 getDim() const { return dim; }

		inline String getTitle() const { return name; }
		inline Flags getFlags() const { return flags; }
		inline u32 getId() const { return id; }

		inline bool visible() const { return flags & VISIBLE; }

	};

	//Implementations

	inline void Window::hide() { flags = Flags(flags & ~VISIBLE); }
	inline void Window::show() { flags = Flags(flags | VISIBLE); }

	inline void Window::setVisible(bool b) {
		if (b) show();
		else hide();
	}

}