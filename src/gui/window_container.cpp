#include "gui/window_container.hpp"
#include "gui/window.hpp"

namespace igx::ui {

	WindowContainer::~WindowContainer() {
		windows.clear();
	}

}