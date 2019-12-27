#pragma once
#include "types/types.hpp"

namespace igx::ui {

	class Window;

	class WindowContainer {

	protected:

		List<Window*> windows;

	public:

		~WindowContainer();

		//Relinquish ownership over window and give it to this UI
		inline bool addWindow(Window *window);

		//Reclaim ownership over window and remove it from the UI (no deletion)
		inline bool removeWindow(Window *window);

		//Delete the window and remove it from the UI
		void deleteWindow(Window *window);

		inline usz size() const { return windows.size(); }

		inline Window **begin() { return windows.data(); }
		inline Window **end() { return begin() + size(); }

		inline Window *operator[](usz i) { return begin()[i]; }

		inline List<Window*> &getWindows() { return windows; }
		inline const List<Window*> &getWindows() const { return windows; }

	};

	//Implementation

	inline bool WindowContainer::addWindow(Window *window) {

		auto it = std::find(windows.begin(), windows.end(), window);

		if (it != windows.end())
			return false;

		windows.push_back(window);
		return true;
	}

	inline bool WindowContainer::removeWindow(Window *window) {

		auto it = std::find(windows.begin(), windows.end(), window);

		if (it == windows.end())
			return false;

		windows.erase(it);
		return true;
	}

}