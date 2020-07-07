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

		struct TemporaryData {
			Buffer heapData;
			u8 data[7]{};
			bool isStillPresent{};
		};

		GUIData *data;

		HashMap<void*, TemporaryData> *map;

		//Recursive structs

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T>>>
		inline void inflect(const String &name, T &t, T2 *parent) {
			doStruct(name);
			t.inflect(*this, parent);
		}

		//Selectors and buttons

		template<typename T, void (T::*x)()>
		inline void inflect(const String&, Button<T, x> &button, T *parent);

		template<typename T>
		inline void inflect(const String &name, bool &checkbox, T*) {
			doCheckbox(name, checkbox);
		}

		template<typename T, typename T2>
		inline void inflect(const String&, Dropdown<T> &dropdown, T2 *parent);

		template<typename T, typename T2>
		inline void inflect(const String&, RadioButtons<T> &radio, T2 *parent);

		//Containers

		template<typename T>
		inline void inflect(const String &name, oic::FileSystem *&fs, T*) {
			doFileSystem(name, fs);
		}

		template<typename T, typename T2>
		inline void inflect(const String &name, List<T> &li, T2*);

		//Integers

		template<typename T> inline void inflect(const String &name, u8 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i8 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u16 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i16 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u32 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i32 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u64 &i, T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i64 &i, T*) { doInt(name, i); }

	private:

		void doCheckbox(const String&, bool &checkbox);
		void doStruct(const String&);

		bool doButton(const String&);
		usz doFileSystem(oic::FileSystem *&fs, const oic::FileHandle handle, const String &path, u32&);
		void doFileSystem(const String&, oic::FileSystem *&fs);

		void doDropdown(const String&, usz &index, const List<const c8*> &names);
		void doRadioButtons(const String&, usz &index, const List<const c8*> &names);

		void *beginList(const String &name, usz count, void *loc);
		void endList(void *ptr);

		void doInt(const String&, isz size, usz requiredBufferSize, void *loc);

		template<typename T>
		inline void doInt(const String &name, T &i) {

			static constexpr isz size = std::is_signed_v<T> ? -isz(sizeof(i)) : isz(sizeof(i));

			//18'446'744'073'709'551'615 or -9'223'372'036'854'775'808

			if constexpr(sizeof(T) == 8)
				doInt(name, size, 20, &i);	

			//These all fit the scheme of unsigned_bits + use_sign

			else {

				static constexpr usz required[] = {
					3,		//255 or 128
					5,		//65'535 or 32'768
					0,
					10		//4'294'967'295 or 2'147'483'648
				};

				doInt(name, size, required[sizeof(i) - 1] + std::is_signed_v<T>, &i);
			}
		}

		//template<typename T, T min, T max, T step>
		//void inflect(MinMaxSlider<T, min, max, step> &slider);
		//
		//template<typename T, T min, T max, T step>
		//void inflect(MinMaxProgress<T, min, max, step> &progress);

	};

	template<typename T>
	struct StructInspector : WindowInterface, ValueContainer<T> {

		HashMap<void*, StructRenderer::TemporaryData> tempData;

		void render(GUIData *data) final override {

			for (auto &elem : tempData)
				elem.second.isStillPresent = false;

			auto inflector = StructRenderer{ data, &tempData };
			value.inflect(inflector, (void*)nullptr);

			List<void*> toDelete;
			toDelete.reserve(tempData.size());

			for (auto &elem : tempData)
				if(!elem.second.isStillPresent)
					toDelete.push_back(elem.first);

			for (auto it = toDelete.rbegin(); it != toDelete.rend(); ++it)
				tempData.erase(*it);
		}
	};

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, List<T> &li, T2*) {

		usz j = li.size();

		if (void *handle = beginList(name, j, &li)) {

			for (usz i = 0; i < j; ++i) {

				//TODO: beginElement

				inflect(std::to_string(i), li[i], &li);

				//TODO: endElement

			}

			endList(handle);
		}
	}

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