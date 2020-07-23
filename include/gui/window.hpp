#pragma once
#include "types/vec.hpp"

namespace igx::ui {

	struct GUIData;

	struct WindowInterface {
		virtual void render(GUIData *data) = 0;
	};

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

			//Runtime modifiers

			HAS_FOCUS			= 1 << 10,

			//Combinations of flags

			NONE						= 0,
			STATIC						= INPUT | HAS_TITLE | BORDER | VISIBLE,
			STATIONARY					= STATIC | MINIMIZE | CLOSE,
			DEFAULT						= MOVE | SCALE | STATIONARY,
			DEFAULT_SCROLL				= DEFAULT | SCROLL,
			DEFAULT_SCROLL_NO_CLOSE		= DEFAULT_SCROLL & ~CLOSE,
			NO_MENU						= INPUT | VISIBLE | MOVE | SCALE,
			STATIC_NO_MENU				= INPUT | VISIBLE
		};

	private:

		Vec2f32 pos, dim;

		String name;

		u32 id;

		Flags flags;

		WindowInterface *interface;

	public:

		//Create a window at a pixel position
		Window(
			const String &name, const u32 id, const Vec2f32 &pos,
			const Vec2f32 &dim, WindowInterface *interface, Flags flags = DEFAULT
		);

		//Setters


		inline void show() { flags = Flags(flags & ~VISIBLE); }
		inline void hide() { flags = Flags(flags | VISIBLE); }
		inline bool isVisible() { return bool(flags & VISIBLE); }

		inline void unfocus() { flags = Flags(flags & ~HAS_FOCUS); }
		inline void focus() { flags = Flags(flags | HAS_FOCUS); }
		inline bool hasFocus() { return bool(flags & HAS_FOCUS); }

		inline void setVisible(bool b) {
			if (b) show();
			else hide();
		}

		inline void setFocus(bool b) {
			if (b) focus();
			else unfocus();
		}

		void updateLocation(const Vec2f32 &po, const Vec2f32 &di) { pos = po; dim = di; }

		//Getters

		inline Vec2f32 getPos() const { return pos; }
		inline Vec2f32 getDim() const { return dim; }

		inline String getTitle() const { return name; }
		inline Flags getFlags() const { return flags; }
		inline u32 getId() const { return id; }

		inline bool visible() const { return flags & VISIBLE; }

		inline void render(GUIData *data) const {
			if (interface) interface->render(data);
		}

		inline bool operator==(const Window &window) const { return id == window.id; }
	};

}