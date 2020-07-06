#pragma once
#include "types/types.hpp"

namespace igx::ui {

	struct ArgHelper {

		static List<String> makeNames(const String &name) {

			List<String> args;
			args.reserve(std::count(name.begin(), name.end(), ','));

			size_t scopes[4]{};
			bool inQuotes[2]{};

			usz lastOccurrence = usz_MAX, end = usz_MAX;
			bool containsNumbers{}, containsLetters{}, isValid = true;

			for (usz i{}, j = name.size(); i < j; ++i) {

				c8 c = name[i];

				switch (c) {

					case ' ': case '\t': case '\r': case '\n': 
						break;

					case '{':	++scopes[0];					isValid = false;	break;
					case '}':	--scopes[0];					isValid = false;	break;
					case '<':	++scopes[1];					isValid = false;	break;
					case '>':	--scopes[1];					isValid = false;	break;
					case '[':	++scopes[2];					isValid = false;	break;
					case ']':	--scopes[2];					isValid = false;	break;
					case '(':	++scopes[3];					isValid = false;	break;
					case ')':	--scopes[3];					isValid = false;	break;

					//Quotes

					case '"':	inQuotes[0] = !inQuotes[0];		isValid = false;	break;

					case '\'':

						isValid = false;

						if (!containsNumbers)				//Numbers only is like 1'000
							inQuotes[1] = !inQuotes[1];

						break;

					//Split commas

					case ',':

						if (
							!scopes[0] && !scopes[1] && 
							!scopes[2] && !scopes[3] && 
							!inQuotes[0] && !inQuotes[1]
						) {
							containsNumbers = containsLetters = false;

							if (isValid && lastOccurrence != usz_MAX) {
								String str = name.substr(lastOccurrence, end - lastOccurrence);
								std::replace(str.begin(), str.end(), '_', ' ');
								args.push_back(str);
								lastOccurrence = end = usz_MAX;
							}

							isValid = true;
						}

						break;

					default:

						if (!isValid) break;

						if (c >= '0' && c <= '9') {

							if (!containsLetters)
								containsNumbers = true;

							else end = i + 1;
						}

						else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {

							containsLetters = true;

							if (lastOccurrence == usz_MAX)
								lastOccurrence = i;
							
							end = i + 1;
						}
				}

			}

			if (isValid && lastOccurrence != usz_MAX) {
				String str = name.substr(lastOccurrence, end - lastOccurrence);
				std::replace(str.begin(), str.end(), '_', ' ');
				args.push_back(str);
			}

			return args;
		}

	};

}

//Either reflection or inspection; inflection

#define Inflect(...)																				\
																									\
template<usz i, typename T, typename T2, typename T3, typename ...args>								\
void _inflect(T &inflector, T2 *parent, const List<String> &names, T3 &t, args &...arg) {			\
																									\
																									\
	if constexpr (sizeof...(args) == 0)																\
		inflector.inflect(names[i], t, parent);														\
																									\
	else {																							\
		inflector.inflect(names[i], t, parent);														\
		_inflect<i + 1>(inflector, parent, names, arg...);											\
	}																								\
}																									\
																									\
template<typename T, typename T2>																	\
void inflect(T &inflector, T2*) {																	\
	static const List<String> namesOfArgs = igx::ui::ArgHelper::makeNames(#__VA_ARGS__);			\
	_inflect<0>(inflector, this, namesOfArgs, __VA_ARGS__);											\
}