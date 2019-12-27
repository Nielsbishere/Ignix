#include "gui/window_container.hpp"
#include "gui/window.hpp"

namespace igx::ui {

	void WindowContainer::deleteWindow(Window *window) {

		if (removeWindow(window))
			delete window;
	}

	WindowContainer::~WindowContainer() {

		for (auto w : windows)
			delete w;

		windows.clear();
	}

}