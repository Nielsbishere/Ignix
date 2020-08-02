#include "gui/struct_inspector.hpp"
#include "system/local_file_system.hpp"

namespace igx::ui {

	using namespace oic;

	bool StructRenderer::doButton(const String &name) {
		nk_layout_row_dynamic(data->ctx, 15, 1);
		return nk_button_label(data->ctx, name.data());
	}

	void StructRenderer::doCheckbox(const String &name, bool &checkbox){

		nk_layout_row_dynamic(data->ctx, 15, 2);

		int active = checkbox;
		nk_checkbox_label(data->ctx, name.c_str(), &active);
		checkbox = active;
	}

	//TODO: nk_hash is 32-bit and not 64-bit

	bool StructRenderer::startStruct(const String &name, const void *addr, usz recursion) {

		if (name.empty()) 
			return true;

		return nk_tree_push_from_hash(
			data->ctx, NK_TREE_TAB, 
			name.c_str(), NK_MAXIMIZED, 
			nk_hash(oic::Hash::collapse32(oic::Hash::hash(usz(addr), recursion)))
		);
	}

	void StructRenderer::endStruct(const String &name) {
		if(name.size())
			nk_tree_pop(data->ctx);
	}

	void StructRenderer::doString(const String &name, WString &str, bool isConst) {

		String intermediate = fromUTF16(str);

		if(isConst)
			doString(name, intermediate, true);
		else {
			doString(name, intermediate, false);
			str = fromUTF8(intermediate);
		}
	}

	void StructRenderer::doString(const String &name, c16 *str, bool isConst, usz maxSize) {

		usz len = std::min(std::char_traits<c16>::length(str), maxSize);

		String intermediate = fromUTF16(WString(str, str + len));

		if(isConst)
			doString(name, intermediate, true);
		else {

			//Since utf16 is involved, only copy the final result if the buffer can fit it

			doString(name, intermediate, false);

			WString result = fromUTF8(intermediate);

			if (result.size() <= maxSize)
				std::memcpy(str, result.c_str(), result.size() * sizeof(c16));
		}
	}

	void StructRenderer::doSliderFloat(const String &name, f64 &a, f64 min, f64 max, f64 step, bool isConst, bool isProgress) {

		//If step is not specified, slider is in perdecamille (1/10th promille)
		//This is so halfs can also represent the numbers correctly
		if (step == 0)
			step = 1e-4 * (max - min);

		if (isProgress) {

			f64 range = (max - min) / step;

			if (std::isnan(range) || std::isinf(range) || range >= f64(u64_MAX)) {
				oic::System::log()->error("Invalid step value");
				return;
			}

		}

		else if (max > f32_MAX) {
			oic::System::log()->error("Nuklear can't represent doubles in sliders");
			return;
		}

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		if (isProgress) {
			nk_size steps = nk_size ((a - min) / step);
			nk_progress(data->ctx, &steps, nk_size((max - min) / step), !isConst);
			a = steps * step + min;
		}
		else {
			f32 v = f32(a);
			nk_slider_float(data->ctx, f32(min), &v, f32(max), f32(step));
			a = v;
		}
	}

	void StructRenderer::doSliderUInt(const String &name, u64 &a, u64 min, u64 max, u64 step, bool isConst, bool isProgress) {

		if (step == 0)
			step = 1;

		if (isProgress) {

			//Only for 32-bit

			if constexpr (sizeof(u64) != sizeof(nk_size)) {
				if ((max - min) > u32_MAX) {
					oic::System::log()->error("Nuklear can't represent values that don't fit into a uint");
					return;
				}
			}

		}

		else if (max >= 1_u64 << (8 * sizeof(int) - 1)) {
			oic::System::log()->error("Nuklear can't represent values that don't fit into an int. Use a progress bar instead");
			return;
		}

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		if (isProgress) {
			nk_size v = nk_size(a - min);
			nk_progress(data->ctx, &v, nk_size(max - min), !isConst);
			a = u64(v) + min;
		}
		else {
			int v = int(a);
			nk_slider_int(data->ctx, int(min), &v, int(max), int(step));
			a = u64(v);
		}
	}

	void StructRenderer::doSliderInt(const String &name, i64 &a, i64 min, i64 max, i64 step, bool isConst, bool isProgress) {

		if (step == 0)
			step = 1;

		if (isProgress) {

			//Only for 32-bit

			if constexpr (sizeof(i64) != sizeof(nk_size)) {
				if (u64(max - min) > i32_MAX) {
					oic::System::log()->error("Nuklear can't represent values that don't fit into a uint");
					return;
				}
			}

		}

		else if (min < i32_MIN || max > i32_MAX) {
			oic::System::log()->error("Nuklear can't represent values that don't fit into an int");
			return;
		}

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		if (isProgress) {
			nk_size v = nk_size(a - min);
			nk_progress(data->ctx, &v, nk_size(max - min), !isConst);
			a = i64(v + min);
		}
		else {
			int v = int(a);
			nk_slider_int(data->ctx, int(min), &v, int(max), int(step));
			a = i64(v);
		}
	}

	void StructRenderer::doVectorHeader(const String &name, usz W, bool isConst) {

		nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, int(W) + int(bool(name.size())));

		if (name.size())
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);

	}

	void StructRenderer::doMatrixHeader(const String &name, usz, usz, bool) {
		nk_layout_row_dynamic(data->ctx, 15.f, 1);
		nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
	}

	void StructRenderer::doString(const String &name, String &str, bool isConst) {

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		if (isConst)
			nk_label(data->ctx, str.c_str(), NK_TEXT_LEFT);
		else {

			auto &temporaryData = map->operator[](&str);
			temporaryData.isStillPresent = true;

			//Setup data and filter out invalid data

			auto &len = *(int*)temporaryData.data;

			usz maxSize = str.capacity() + 9;

			if (
				len != str.size() ||
				temporaryData.heapData.size() < maxSize || 
				std::memcmp(temporaryData.heapData.data(), str.data(), str.size())
				) {

				temporaryData.heapData.resize(maxSize);
				std::memcpy(temporaryData.heapData.data(), str.data(), str.size());

				temporaryData.heapData[str.size()] = 0;

				len = (int) str.size();
			}

			c8 *heapPtr = (c8*)temporaryData.heapData.data();

			nk_edit_string(
				data->ctx, NK_EDIT_FIELD, 
				heapPtr, 
				&len, (int)str.capacity() + 8, 
				nk_filter_default
			);

			String modified(heapPtr, heapPtr + len);

			if(str != modified)
				str = modified;
		}
	}

	void StructRenderer::doString(const String &name, c8 *str, bool isConst, usz maxSize) {

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		int len = (int) std::min(strlen(str), maxSize);

		if (isConst)
			nk_label(data->ctx, String(str, str + len).c_str(), NK_TEXT_LEFT);
		else 
			nk_edit_string(
				data->ctx, NK_EDIT_FIELD, 
				(char*)str, 
				&len, (int)maxSize, 
				nk_filter_default
			);
	}

	template<bool isFloat = false>
	inline String stringify(const u8 *loc, isz byteSize, NumberFormat numberFormat) {

		std::stringstream ss;

		static constexpr c8 mapping[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

		switch(numberFormat){

		case NumberFormat::HEX:

			ss << '#';

			for (usz i = usz(std::abs(byteSize)) - 1; i != usz_MAX; --i) {
				u8 val = loc[i];
				ss << mapping[val >> 4] << mapping[val & 0xF];
			}

			break;

		case NumberFormat::BIN:

			ss << "0b";

			for (usz i = (usz(std::abs(byteSize)) >> 3) - 1; i != usz_MAX; --i)
				ss << u32(bool(loc[i >> 3] & (1 << (i & 7))));

			break;

		case NumberFormat::OCT:
			oic::System::log()->fatal("Octal stringify not supported yet");		//TODO: Octal

		default:

			if constexpr(isFloat)
				switch (byteSize) {
				case 2:		ss << f32(*(f16*)loc);	break;
				case 4:		ss << *(f32*)loc;		break;
				case 8:		ss << *(f64*)loc;
				}
			else
				switch (byteSize) {
				case 1: ss << (u16)*(u8*)loc;		break;
				case -1: ss << (i16)*(i8*)loc;		break;
				case 2: ss << *(u16*)loc;			break;
				case -2: ss << *(i16*)loc;			break;
				case 4: ss << *(u32*)loc;			break;
				case -4: ss << *(i32*)loc;			break;
				case 8: ss << *(u64*)loc;			break;
				case -8: ss << *(i64*)loc;
				}
		}

		return ss.str();
	}

	inline bool integerify(i64 &out, const String &str, NumberFormat numberFormat, bool allowNeg) {

		if (str.empty())
			return false;

		i64 sign = 1;
		u64 res{};
		usz j = str.size(), end = usz_MAX;

		if (numberFormat == NumberFormat::HEX && str[0] == '#') {

			end = 0;

			if (j == 1)
				return false;
		}

		if (numberFormat == NumberFormat::BIN && str.size() >= 2 && str[0] == '0' && str[1] == 'b') {

			end = 1;

			if (j == 2)
				return false;
		}

		if (numberFormat == NumberFormat::DEC && str[0] == '-') {

			if (!allowNeg)
				return false;

			sign *= -1;
			end = 0;

			if (j == 1)
				return false;
		}

		i64 mul = 1;

		static constexpr u64 exps[] = { 10, 16, 8, 2 };

		u64 ex = exps[usz(numberFormat)];

		for (usz i = j - 1; i != end; --i) {

			switch (str[i]) {

			case 'F': case 'E': case 'D': case 'C': case 'B': case 'A':

				if (numberFormat != NumberFormat::HEX)
					return false;

				res += mul * (str[i] - ('A' - 10));
				mul *= 16;
				break;

			case '9': case '8':

				if (numberFormat == NumberFormat::OCT)
					return false;

			case '7': case '6': case '5': case '4': case '3': case '2':

				if (numberFormat == NumberFormat::BIN)
					return false;

			case '1': case '0':

				res += mul * (str[i] - '0');
				mul *= ex;
				break;

			default:
				return false;
			}
		}

		out = i64(res) * sign;
		return true;
	}

	void StructRenderer::doFloat(const String &name, usz byteSize, const void *loc, bool isConst, NumberFormat numberFormat) {

		if (isConst) {

			if (name.size()) {
				nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
				nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
			}

			nk_label(data->ctx, stringify<true>((const u8*)loc, byteSize, numberFormat).c_str(), NK_TEXT_LEFT);
			return;
		}

		auto &temporaryData = map->operator[](loc);
		temporaryData.isStillPresent = true;

		//Setup data and filter out invalid data

		auto &len = *(int*)temporaryData.data;
		static constexpr usz required = 64;

		if (
			temporaryData.heapData.size() != required + byteSize + 1 || 
			std::memcmp(temporaryData.heapData.data(), loc, byteSize)
			) {

			//Reset data

			temporaryData.heapData.resize(required + byteSize + 1);
			std::memcpy(temporaryData.heapData.data(), loc, byteSize);

			//Init string

			String str = stringify<true>((const u8*)loc, byteSize, numberFormat);

			oicAssert("Unexpected float string out of bounds", str.size() <= required);

			//Copy and set null to the end of the string

			std::memcpy(temporaryData.heapData.data() + byteSize, str.data(), str.size());

			usz leftOver = required - str.size() + 1;

			if(leftOver)
				std::memset(temporaryData.heapData.data() + byteSize + str.size(), 0, leftOver);

			//Store length

			len = (int) str.size();
		}

		if (len < 0 || len > required)
			len = 0;

		//TODO: If a float ends with . please don't replace it, it's annoying

		//Edit string

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		static constexpr nk_plugin_filter filters[] = {
			nk_filter_float,
			nk_filter_hex,
			nk_filter_oct,
			nk_filter_binary
		};

		c8 *start = (c8*)(temporaryData.heapData.data() + byteSize);
		String prev = String(start, start + len);

		//TODO: With hex values this doesn't work?

		nk_edit_string(
			data->ctx, NK_EDIT_FIELD, 
			start, 
			&len, (int)required + 1, 
			filters[usz(numberFormat)]
		);

		//Output

		String newStr = String(start, start + len);

		if (newStr == prev)
			return;

		if (numberFormat == NumberFormat::DEC) {

			char *err{};
			f64 val = strtod(newStr.c_str(), &err);

			if(err)
				switch (byteSize) {
				case 2:		*(f16*)loc = f32(val);	break;
				case 4:		*(f32*)loc = f32(val);	break;
				case 8:		*(f64*)loc = val;
				}

			return;
		}

		i64 val;

		if (!integerify(val, newStr, numberFormat, byteSize < 0))
			return;

		std::stringstream ss(newStr);

		switch (byteSize) {
		case 2:		*(f16*)loc = *(f16*)&val;	break;
		case 4:		*(f32*)loc = *(f32*)&val;	break;
		case 8:		*(f64*)loc = *(f64*)&val;
		}
	}

	void StructRenderer::doInt(const String &name, isz byteSize, usz required, const void *loc, bool isConst, NumberFormat numberFormat) {

		if (isConst) {

			if (name.size()) {
				nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
				nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
			}

			nk_label(data->ctx, stringify((const u8*)loc, byteSize, numberFormat).c_str(), NK_TEXT_LEFT);
			return;
		}

		auto &temporaryData = map->operator[](loc);
		temporaryData.isStillPresent = true;

		//Setup data and filter out invalid data

		usz ubyteSize = std::abs(byteSize);
		auto &len = *(int*)temporaryData.data;

		if (
			temporaryData.heapData.size() != required + ubyteSize + 1 || 
			std::memcmp(temporaryData.heapData.data(), loc, ubyteSize)
			) {

			//Reset data

			temporaryData.heapData.resize(required + ubyteSize + 1);
			std::memcpy(temporaryData.heapData.data(), loc, ubyteSize);

			//Init string

			String str = stringify((const u8*)loc, byteSize, numberFormat);

			oicAssert("Unexpected int string out of bounds", str.size() <= required);

			//Copy and set null to the end of the string

			std::memcpy(temporaryData.heapData.data() + ubyteSize, str.data(), str.size());

			usz leftOver = required - str.size() + 1;

			if(leftOver)
				std::memset(temporaryData.heapData.data() + ubyteSize + str.size(), 0, leftOver);

			//Store length

			len = (int) str.size();
		}

		if (len < 0 || len > required)
			len = 0;

		//TODO: Ignore - if unsigned, otherwise only allow at front

		//Edit string

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, isConst ? 15.f : 20.f, 2);
			nk_label(data->ctx, name.c_str(), NK_TEXT_LEFT);
		}

		static constexpr nk_plugin_filter filters[] = {
			nk_filter_decimal,
			nk_filter_hex,
			nk_filter_oct,
			nk_filter_binary
		};

		c8 *start = (c8*)(temporaryData.heapData.data() + ubyteSize);
		String prev = String(start, start + len);

		//TODO: With hex values this doesn't work?

		nk_edit_string(
			data->ctx, NK_EDIT_FIELD, 
			start, 
			&len, (int)required + 1, 
			filters[usz(numberFormat)]
		);

		//Output

		String newStr = String(start, start + len);

		if (newStr == prev)
			return;

		i64 val;

		if (!integerify(val, newStr, numberFormat, byteSize < 0))
			return;

		std::stringstream ss(newStr);

		switch (byteSize) {

		case 1: *(u8*)loc = u8(val);		break;
		case -1: *(i8*)loc = i8(val);		break;
		case 2: *(u16*)loc = u16(val);		break;
		case -2: *(i16*)loc = i16(val);		break;
		case 4: *(u32*)loc = u32(val);		break;
		case -4: *(i32*)loc = i32(val);		break;
		case 8: *(u64*)loc = u64(val);		break;
		case -8: *(i64*)loc = val;
		}
	}

	void *StructRenderer::beginList(const String &name, usz count, bool isInlineEditable, const void *loc) {

		//TODO: Scrollbar

		f32 lineHeight = isInlineEditable ? 20.f : 15.f;
		nk_layout_row_dynamic(data->ctx, isInlineEditable ? lineHeight * (count + 1) : 200, 1);

		//Get nk_list_view from temporary data

		auto &temporaryData = map->operator[](loc);
		temporaryData.isStillPresent = true;

		if (temporaryData.heapData.size() != sizeof(nk_list_view))
			temporaryData.heapData.resize(sizeof(nk_list_view));

		auto v = (nk_list_view*)temporaryData.heapData.data();

		//(No need to clear since nk_list_view_begin sets it

		auto flags = NK_WINDOW_BORDER;

		if (isInlineEditable)
			flags = nk_panel_flags(flags | NK_WINDOW_NO_SCROLLBAR);

		if (nk_list_view_begin(data->ctx, (nk_list_view*)v, name.c_str(), flags, int(lineHeight), int(count))) {

			if(isInlineEditable)
				nk_layout_row_dynamic(data->ctx, lineHeight, 2);

			return v;
		}

		return nullptr;
	}

	void StructRenderer::endList(void *ptr) {
		nk_list_view_end((nk_list_view*)ptr);
	}

	//TODO: size of combo box and of line should be changable in a layout or something

	void StructRenderer::doDropdown(const String &name, usz &index, const List<const c8*> &names) {

		auto *ctx = data->ctx;

		int selected = int(index);

		if (selected < 0 || selected >= names.size()) 
			selected = 0;		//Avoid garbage data

								//TODO: Opening two after each other seems to infinite loop?

		nk_layout_row_dynamic(data->ctx, 15, 1 + bool(name.size()));

		if (name.size())
			nk_label(data->ctx, name.c_str(), NK_TEXT_ALIGN_LEFT);

		nk_combobox(ctx, names.data(), int(names.size()), &selected, 15, nk_vec2(200, 300));

		index = usz(selected);
	}

	void StructRenderer::doRadioButtons(const String &name, usz &index, const List<const c8*> &names) {

		if (name.size()) {
			nk_layout_row_dynamic(data->ctx, 15, 1);
			nk_label(data->ctx, name.c_str(), NK_TEXT_ALIGN_LEFT);
		}

		nk_layout_row_static(data->ctx, 15, 75, 2);

		for (usz i = 0; i < names.size(); ++i)
			if (nk_option_label(data->ctx, names[i], index == i))
				index = i;

	}

	usz StructRenderer::doFileSystem(const oic::FileSystem *fs, oic::FileHandle handle, const String &path, u32 &selected, bool maximized) {

		oicAssert("Local file system is not supported yet", path.empty());

		FileInfo info = fs->getVirtualFiles()[handle];

		if (nk_tree_element_push_id(
			data->ctx, 
			info.isFolder() ? NK_TREE_NODE : NK_TREE_CHILD, 
			info.name.c_str(),
			maximized ? NK_MAXIMIZED : NK_MINIMIZED, 
			(int*)&selected, (int)handle
		)) {

			if (info.isFolder())
				for (FileHandle i = info.folderHint; i != info.fileEnd; ++i)
					doFileSystem(fs, i, path, selected);

			nk_tree_pop(data->ctx);
		}

		return handle;
	}

	void StructRenderer::doFileSystem(const String &name, const oic::FileSystem *&fs) {

		if (nk_tree_push_from_hash(data->ctx, NK_TREE_TAB, name.c_str(), NK_MAXIMIZED, nk_hash(usz(&fs)))) {

			if (fs) {

				//Reserve temporary data

				auto &temporaryData = map->operator[](&fs);
				temporaryData.isStillPresent = true;

				//Go through virtual path

				doFileSystem(fs, 0, "", *(u32*)temporaryData.data, true);

			}

			nk_tree_pop(data->ctx);
		}

	}

}