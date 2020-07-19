#pragma once
#include "ui_value.hpp"
#include "gui.hpp"
#include "utils/inflect.hpp"

namespace oic {
	class FileSystem;
	using FileHandle = u32;
}

#ifndef _inline_
	#ifdef _MSC_VER
		#define _inline_ __forceinline
	#endif
#endif

namespace igx::ui {

	struct StructRenderer {

		struct TemporaryData {
			Buffer heapData;
			u8 data[7]{};
			bool isStillPresent{};
		};

		GUIData *data;

		HashMap<const void*, TemporaryData> *map;

		//Recursive structs

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T>>>
		inline void inflect(const String &name, T &t, const T2 *parent) {
			if (startStruct(name)) {
				t.inflect(*this, parent);
				endStruct();
			}
		}

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T>>>
		inline void inflect(const String &name, const T &t, const T2 *parent) {
			if (startStruct(name)) {
				t.inflect(*this, parent);
				endStruct();
			}
		}

		//Selectors and buttons

		template<typename T, void (T::*x)() const>
		inline void inflect(const String&, const Button<T, x> &button, const T *parent);

		template<typename T>
		inline void inflect(const String &name, bool &checkbox, const T*) {
			doCheckbox(name, checkbox);
		}

		template<typename T, typename T2>
		inline void inflect(const String&, Dropdown<T> &dropdown, const T2 *parent);

		template<typename T, typename T2>
		inline void inflect(const String&, RadioButtons<T> &radio, const T2 *parent);

		//Containers

		template<typename T>
		inline void inflect(const String &name, const oic::FileSystem *fs, const T*) {
			doFileSystem(name, fs);
		}

		template<typename T, typename T2, bool isConst = false>
		inline void inflect(const String &name, List<T> &li, const T2*);

		template<typename K, typename V, typename T2, bool isConst = false>
		inline void inflect(const String &name, HashMap<K, V> &li, const T2*);

		template<typename T, typename T2>
		inline void inflect(const String &name, const List<T> &li, const T2*);

		template<typename K, typename V, typename T2>
		inline void inflect(const String &name, const HashMap<K, V> &li, const T2*);

		//Base types

		template<typename T> inline void inflect(const String &name, u8 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i8 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u16 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i16 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u32 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i32 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, u64 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, i64 &i, const T*) { doInt(name, i); }

		template<typename T> inline void inflect(const String &name, const u8 &i, const T*) { doInt(name, (u8&) i, true); }
		template<typename T> inline void inflect(const String &name, const i8 &i, const T*) { doInt(name, (i8&) i, true); }
		template<typename T> inline void inflect(const String &name, const u16 &i, const T*) { doInt(name, (u16&) i, true); }
		template<typename T> inline void inflect(const String &name, const i16 &i, const T*) { doInt(name, (i16&) i, true); }
		template<typename T> inline void inflect(const String &name, const u32 &i, const T*) { doInt(name, (u32&) i, true); }
		template<typename T> inline void inflect(const String &name, const i32 &i, const T*) { doInt(name, (i32&) i, true); }
		template<typename T> inline void inflect(const String &name, const u64 &i, const T*) { doInt(name, (u64&) i, true); }
		template<typename T> inline void inflect(const String &name, const i64 &i, const T*) { doInt(name, (i64&) i, true); }

		template<typename T> inline void inflect(const String &name, String &str, const T*) { doString(name, str, false); }
		template<typename T> inline void inflect(const String &name, const String &str, const T*) { doString(name, (String&) str, true); }

		template<typename T> inline void inflect(const String &name, c8 *&str, const T*) { doString(name, (c8*)str, false, strlen(str)); }
		template<typename T> inline void inflect(const String &name, const c8 *&str, const T*) { doString(name, (c8*)str, true, strlen(str)); }

		template<typename T, usz siz> inline void inflect(const String &name, c8 (&str)[siz], const T*) { doString(name, (c8*)str, false, siz); }
		template<typename T, usz siz> inline void inflect(const String &name, const c8 (&str)[siz], const T*) { doString(name, (c8*)str, true, siz); }

		//Structs

		template<usz i = 0, typename T, typename T2, typename ...args>
		_inline_ void inflect(const T *parent, const List<String> &names, T2 &t, args &&...arg) {

			if constexpr (sizeof...(args) == 0)
				inflect(names[i], t, parent);
			else {
				inflect(names[i], t, parent);	
				inflect<i + 1>(parent, names, arg...);
			}
		}

		template<usz i = 0, typename T, typename T2, typename ...args>
		_inline_ void inflect(const T *parent, const List<String> &names, const T2 &t, args &&...arg) {

			if constexpr (sizeof...(args) == 0)
				inflect(names[i], t, parent);
			else {
				inflect(names[i], t, parent);	
				inflect<i + 1>(parent, names, arg...);
			}
		}

	private:

		void doCheckbox(const String&, bool &checkbox);
		bool startStruct(const String&);
		void endStruct();

		void doString(const String&, String &str, bool isConst);
		void doString(const String&, c8 *str, bool isConst, usz maxSize = usz_MAX);

		bool doButton(const String&);
		usz doFileSystem(const oic::FileSystem *fs, const oic::FileHandle handle, const String &path, u32&);
		void doFileSystem(const String&, const oic::FileSystem *fs);

		void doDropdown(const String&, usz &index, const List<const c8*> &names);
		void doRadioButtons(const String&, usz &index, const List<const c8*> &names);

		void *beginList(const String &name, usz count, const void *loc);
		void endList(void *ptr);

		void doInt(const String&, isz size, usz requiredBufferSize, const void *loc, bool isConst);

		template<typename T>
		inline void doInt(const String &name, T &i, bool isConst = false) {

			static constexpr isz size = std::is_signed_v<T> ? -isz(sizeof(i)) : isz(sizeof(i));

			//18'446'744'073'709'551'615 or -9'223'372'036'854'775'808

			if constexpr(sizeof(T) == 8)
				doInt(name, size, 20, &i, isConst);	

			//These all fit the scheme of unsigned_bits + use_sign

			else {

				static constexpr usz required[] = {
					3,		//255 or 128
					5,		//65'535 or 32'768
					0,
					10		//4'294'967'295 or 2'147'483'648
				};

				doInt(name, size, required[sizeof(i) - 1] + std::is_signed_v<T>, &i, isConst);
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

		HashMap<const void*, StructRenderer::TemporaryData> tempData;

		using ValueContainer<T>::ValueContainer;

		void render(GUIData *data) final override {

			for (auto &elem : tempData)
				elem.second.isStillPresent = false;

			auto inflector = StructRenderer{ data, &tempData };
			value.inflect(inflector, (void*)nullptr);

			List<const void*> toDelete;
			toDelete.reserve(tempData.size());

			for (auto &elem : tempData)
				if(!elem.second.isStillPresent)
					toDelete.push_back(elem.first);

			for (auto it = toDelete.rbegin(); it != toDelete.rend(); ++it)
				tempData.erase(*it);
		}
	};

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, const List<T> &li, const T2 *parent) {
		inflect<true>(name, (List<T>&) li, parent);
	}

	template<typename K, typename V, typename T2>
	inline void StructRenderer::inflect(const String &name, const HashMap<K, V> &li, const T2*) {

		usz j = li.size();

		if (void *handle = beginList(name, j, &li)) {

			for (auto &elem : li) {

				//TODO: beginElement

				inflect("", elem.first, &li);
				inflect("", elem.second, &li);

				//TODO: endElement

			}

			endList(handle);
		}
	}

	template<typename T, typename T2, bool isConst>
	inline void StructRenderer::inflect(const String &name, List<T> &li, const T2*) {

		usz j = li.size();

		if (void *handle = beginList(name, j, &li)) {

			for (usz i = 0; i < j; ++i) {

				//TODO: beginElement

				if constexpr(isConst)
					inflect(std::to_string(i), (const T&) li[i], &li);
				else
					inflect(std::to_string(i), li[i], &li);

				//TODO: endElement

			}

			endList(handle);
		}
	}

	template<typename K, typename V, typename T2, bool isConst>
	inline void StructRenderer::inflect(const String &name, HashMap<K, V> &li, const T2*) {

		usz j = li.size();

		if (void *handle = beginList(name, j, &li)) {

			for (auto &elem : li) {

				//TODO: beginElement

				if constexpr (isConst) {
					inflect("", (const K&) elem.first, &li);
					inflect("", (const V&) elem.second, &li);
				}
				else {
					inflect("", elem.first, &li);
					inflect("", elem.second, &li);
				}

				//TODO: endElement

			}

			endList(handle);
		}
	}

	template<typename T, void (T::*x)() const>
	inline void StructRenderer::inflect(const String &name, const Button<T, x> &button, const T *parent) {
		if (doButton(name))
			button.call(parent);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, Dropdown<T> &dropdown, const T2*) {
		usz id = dropdown.id();
		doDropdown(name, id, dropdown.names());
		dropdown.setId(id);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, RadioButtons<T> &radio, const T2*) {
		usz id = radio.id();
		doRadioButtons(name, id, radio.names());
		radio.setId(id);
	}

}