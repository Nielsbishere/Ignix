#pragma once
#include "window.hpp"
#include <algorithm>

namespace igx::ui {

	class WindowContainer {

	protected:

		List<Window> windows;

	public:

		~WindowContainer();

		inline bool addWindow(const Window &window);
		inline bool removeWindow(u32 id);

		inline usz size() const { return windows.size(); }

		inline auto begin() { return windows.data(); }
		inline auto end() { return begin() + size(); }
		inline auto begin() const { return windows.data(); }
		inline auto end() const { return begin() + size(); }

		inline auto &operator[](usz i) { return begin()[i]; }
		inline auto &operator[](usz i) const { return begin()[i]; }
	};

	//Implementation

	inline bool WindowContainer::addWindow(const Window &window) {

		auto it = std::find(windows.begin(), windows.end(), window);

		if (it != windows.end())
			return false;

		windows.push_back(window);
		return true;
	}

	inline bool WindowContainer::removeWindow(u32 id) {

		auto it = std::find_if(windows.begin(), windows.end(), [id](const Window &w) -> bool { return w.getId() == id;  });

		if (it == windows.end())
			return false;

		windows.erase(it);
		return true;
	}

}