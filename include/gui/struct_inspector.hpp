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

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T> || std::is_union_v<T>>>
		inline void inflect(const String &name, usz recursion, T &t, const T2 *parent) {
			if (startStruct(name, &t, recursion)) {
				t.inflect(*this, recursion, parent);
				endStruct(name);
			}
		}

		template<typename T, typename T2, typename = std::enable_if_t<std::is_class_v<T> || std::is_union_v<T>>>
		inline void inflect(const String &name, usz recursion, const T &t, const T2 *parent) {
			if (startStruct(name, &t, recursion)) {
				t.inflect(*this, recursion, parent);
				endStruct(name);
			}
		}

		//Selectors and buttons

		template<typename T, void (T::*x)() const>
		inline void inflect(const String&, usz, const Button<T, x> &button, const T *parent);

		template<typename T>
		inline void inflect(const String &name, usz, bool &checkbox, const T*) {
			doCheckbox(name, checkbox);
		}

		template<typename T, typename T2>
		inline void inflect(const String&, usz, Dropdown<T> &dropdown, const T2 *parent);

		template<typename T, typename T2>
		inline void inflect(const String&, usz, RadioButtons<T> &radio, const T2 *parent);

		//Containers

		template<typename T>
		inline void inflect(const String &name, usz, const oic::FileSystem *&fs, const T*) {
			doFileSystem(name, fs);
		}

		template<typename T>
		inline void inflect(const String &name, usz, oic::FileSystem *&fs, const T*) {
			doFileSystem(name, (const oic::FileSystem*&) fs);
		}

		template<typename T, typename T2, bool isConst = false>
		inline void inflect(const String &name, usz, List<T> &li, const T2*);

		template<typename K, typename V, typename T2, bool isConst = false>
		inline void inflect(const String &name, usz, HashMap<K, V> &li, const T2*);

		template<typename T, typename T2>
		inline void inflect(const String &name, usz, const List<T> &li, const T2*);

		template<typename K, typename V, typename T2>
		inline void inflect(const String &name, usz, const HashMap<K, V> &li, const T2*);

		//Base types

		template<typename T> inline void inflect(const String &name, usz, u8 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, i8 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, u16 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, i16 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, u32 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, i32 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, u64 &i, const T*) { doInt(name, i); }
		template<typename T> inline void inflect(const String &name, usz, i64 &i, const T*) { doInt(name, i); }

		template<typename T> inline void inflect(const String &name, usz, const u8 &i, const T*) { doInt(name, (u8&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const i8 &i, const T*) { doInt(name, (i8&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const u16 &i, const T*) { doInt(name, (u16&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const i16 &i, const T*) { doInt(name, (i16&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const u32 &i, const T*) { doInt(name, (u32&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const i32 &i, const T*) { doInt(name, (i32&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const u64 &i, const T*) { doInt(name, (u64&) i, true); }
		template<typename T> inline void inflect(const String &name, usz, const i64 &i, const T*) { doInt(name, (i64&) i, true); }

		//Numbers formatted differently

		template<typename T, typename T2, NumberFormat Format> 
		inline void inflect(const String &name, usz, Val<T, Format> &format, const T2*) { doInt<Format>(name, format.value, false); }

		template<typename T, typename T2, NumberFormat Format> 
		inline void inflect(const String &name, usz, const Val<T, Format> &format, const T2*) { doInt<Format>(name, (T&)format.value, true); }

		//Strings

		template<typename T> inline void inflect(const String &name, usz, String &str, const T*) { doString(name, str, false); }
		template<typename T> inline void inflect(const String &name, usz, const String &str, const T*) { doString(name, (String&) str, true); }

		template<typename T> inline void inflect(const String &name, usz, c8 *&str, const T*) { doString(name, (c8*)str, false, strlen(str)); }
		template<typename T> inline void inflect(const String &name, usz, const c8 *&str, const T*) { doString(name, (c8*)str, true, strlen(str)); }

		template<typename T, usz siz> inline void inflect(const String &name, usz, c8 (&str)[siz], const T*) { doString(name, (c8*)str, false, siz); }
		template<typename T, usz siz> inline void inflect(const String &name, usz, const c8 (&str)[siz], const T*) { doString(name, (c8*)str, true, siz); }

		template<typename T> inline void inflect(const String &name, usz, WString &str, const T*) { doString(name, str, false); }
		template<typename T> inline void inflect(const String &name, usz, const WString &str, const T*) { doString(name, (WString&) str, true); }

		template<typename T> inline void inflect(const String &name, usz, c16 *&str, const T*) { doString(name, (c16*)str, false, wcslen(str)); }
		template<typename T> inline void inflect(const String &name, usz, const c16 *&str, const T*) { doString(name, (c16*)str, true, wcslen(str)); }

		template<typename T, usz siz> 
		inline void inflect(const String &name, usz, c16 (&str)[siz], const T*) { doString(name, (c16*)str, false, siz); }

		template<typename T, usz siz> 
		inline void inflect(const String &name, usz, const c16 (&str)[siz], const T*) { doString(name, (c16*)str, true, siz); }

		//Structs

		template<usz i = 0, typename T, typename T2, typename ...args>
		_inline_ void inflect(const T *parent, usz recursion, const List<String> &names, T2 &t, args &&...arg) {

			if constexpr (sizeof...(args) == 0)
				inflect(names[i], recursion + 1, t, parent);
			else {
				inflect(names[i], recursion + 1, t, parent);	
				inflect<i + 1>(parent, recursion, names, arg...);
			}
		}

		template<usz i = 0, typename T, typename T2, typename ...args>
		_inline_ void inflect(const T *parent, usz recursion, const List<String> &names, const T2 &t, args &&...arg) {

			if constexpr (sizeof...(args) == 0)
				inflect(names[i], recursion + 1, t, parent);
			else {
				inflect(names[i], recursion + 1, t, parent);	
				inflect<i + 1>(parent, recursion, names, arg...);
			}
		}

	private:

		void doCheckbox(const String&, bool &checkbox);
		bool startStruct(const String&, const void *addr, usz recursion);
		void endStruct(const String&);

		void doString(const String&, String &str, bool isConst);
		void doString(const String&, c8 *str, bool isConst, usz maxSize = usz_MAX);

		void doString(const String&, WString &str, bool isConst);
		void doString(const String&, c16 *str, bool isConst, usz maxSize = usz_MAX);

		bool doButton(const String&);
		usz doFileSystem(const oic::FileSystem *fs, const oic::FileHandle handle, const String &path, u32&, bool maximized = false);
		void doFileSystem(const String&, const oic::FileSystem *&fs);

		void doDropdown(const String&, usz &index, const List<const c8*> &names);
		void doRadioButtons(const String&, usz &index, const List<const c8*> &names);

		void *beginList(const String &name, usz count, const void *loc);
		void endList(void *ptr);

		void doInt(const String&, isz size, usz requiredBufferSize, const void *loc, bool isConst, NumberFormat numberFormat);

		template<NumberFormat numberFormat = NumberFormat::DEC, typename T>
		inline void doInt(const String &name, T &i, bool isConst = false) {

			static constexpr isz size = std::is_signed_v<T> ? -isz(sizeof(i)) : isz(sizeof(i));

			usz required;

			//Binary formats

			if constexpr (numberFormat == NumberFormat::HEX)		//2 characters per byte		+ #
				required = 1 + sizeof(i) * 2;

			else if constexpr (numberFormat == NumberFormat::BIN)	//8 characters per byte		+ 0b
				required = 2 + sizeof(i) * 8;

			else if constexpr (numberFormat == NumberFormat::OCT)	//1 character per 3 bits	+ 0
				required = 1 + usz(std::ceil(sizeof(i) * 8 / 3.f));

			//Decimal

			//18'446'744'073'709'551'615 or -9'223'372'036'854'775'808

			else if constexpr (sizeof(T) == 8)
				required = 20;

			//These all fit the scheme of unsigned_bits + use_sign

			else {

				static constexpr usz bySize[] = {
					3,		//255 or 128
					5,		//65'535 or 32'768
					0,
					10		//4'294'967'295 or 2'147'483'648
				};

				required = bySize[sizeof(i) - 1] + std::is_signed_v<T>;
			}

			//Do an int

			doInt(name, size, required, &i, isConst, numberFormat);
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

			inflector.inflect("", 0, value, (void*)nullptr);

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
	inline void StructRenderer::inflect(const String &name, usz, const List<T> &li, const T2 *parent) {
		inflect<true>(name, (List<T>&) li, parent);
	}

	template<typename K, typename V, typename T2>
	inline void StructRenderer::inflect(const String &name, usz, const HashMap<K, V> &li, const T2*) {

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
	inline void StructRenderer::inflect(const String &name, usz, List<T> &li, const T2*) {

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
	inline void StructRenderer::inflect(const String &name, usz, HashMap<K, V> &li, const T2*) {

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
	inline void StructRenderer::inflect(const String &name, usz, const Button<T, x> &button, const T *parent) {
		if (doButton(name))
			button.call(parent);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, usz, Dropdown<T> &dropdown, const T2*) {
		usz id = dropdown.id();
		doDropdown(name, id, dropdown.names());
		dropdown.setId(id);
	}

	template<typename T, typename T2>
	inline void StructRenderer::inflect(const String &name, usz, RadioButtons<T> &radio, const T2*) {
		usz id = radio.id();
		doRadioButtons(name, id, radio.names());
		radio.setId(id);
	}

}