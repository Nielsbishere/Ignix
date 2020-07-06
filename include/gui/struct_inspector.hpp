#pragma once
#include "ui_value.hpp"
#include "gui.hpp"
#include "utils/inflect.hpp"

namespace oic {
	class FileSystem;
	using FileHandle = u32;
}

namespace igx::ui {

	struct StructRenderer {

		GUIData *data;

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T>>>
		inline void inflect(const String&, T &t, T2 *parent) {
			t.inflect(*this, parent);
		}

		template<typename T, void (T::*x)()>
		inline void inflect(const String&, Button<T, x> &button, T *parent);

		void doCheckbox(const String&, Checkbox &checkbox);

		template<typename T>
		inline void inflect(const String &name, Checkbox &checkbox, T*) {
			doCheckbox(name, checkbox);
		}

		bool doButton(const String&);
		usz doFileSystem(oic::FileSystem *&fs, const oic::FileHandle handle, const String &path, usz&);
		void doFileSystem(const String&, oic::FileSystem *&fs);

		void doDropdown(const String&, usz &index, const List<const c8*> &names);
		void doRadioButtons(const String&, usz &index, const List<const c8*> &names);

		template<typename T, typename T2>
		inline void inflect(const String&, Dropdown<T> &dropdown, T2 *parent);

		template<typename T, typename T2>
		inline void inflect(const String&, RadioButtons<T> &radio, T2 *parent);

		template<typename T>
		inline void inflect(const String &name, oic::FileSystem *&fs, T*) {
			doFileSystem(name, fs);
		}

		//template<typename T, T min, T max, T step>
		//void inflect(MinMaxSlider<T, min, max, step> &slider);
		//
		//template<typename T, T min, T max, T step>
		//void inflect(MinMaxProgress<T, min, max, step> &progress);

	};

	template<typename T>
	struct StructInspector : WindowInterface, ValueContainer<T> {
		void render(GUIData *data) final override {
			auto inflector = StructRenderer{ data };
			value.inflect(inflector, (void*)nullptr);
		}
	};

	template<typename T, void (T::*x)()>
	inline void StructRenderer::inflect(const String &name, Button<T, x> &button, T *parent) {
		if (doButton(name))
			button.call(parent);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, Dropdown<T> &dropdown, T2*) {
		usz id = dropdown.id();
		doDropdown(name, id, dropdown.names());
		dropdown.setId(id);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, RadioButtons<T> &radio, T2*) {
		usz id = radio.id();
		doRadioButtons(name, id, radio.names());
		radio.setId(id);
	}

}