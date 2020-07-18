#pragma once
#include "types/types.hpp"
#include "utils/math.hpp"

namespace igx::ui {

	//Helper functions

	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	static constexpr T minValue() {

		if constexpr (std::is_floating_point_v<T>)
			return -std::numeric_limits<T>::max();

		else if constexpr (std::is_unsigned_v<T>)
			return 0;

		else return T(1 << ((u64(sizeof(T)) << 3) - 1));
	}

	template<typename T, T min, T max>
	static inline constexpr T clamp(T val) {

		static constexpr bool hasMax = max != std::numeric_limits<T>::max();
		static constexpr bool hasMin = min != minValue<T>();

		if constexpr (hasMin && hasMax)
			return val <= min ? min : (val >= max ? max : val);

		else if constexpr (hasMin)
			return val <= min ? min : val;

		else if constexpr (hasMax)
			return val <= min ? min : val;

		else return val;
	}

	template<typename T>
	static inline constexpr T mod(T t, T val) {

		if constexpr (std::is_integral_v<T>)
			return t % val;

		else return t - std::floor(t / val) * val;
	}

	template<typename T>
	static inline constexpr T hat(T t, T val) {

		if constexpr (std::is_integral_v<T>)
			return t ^ val;

		else return std::pow(t, val);
	}

	//Sliders

	template<typename T, T min, T max, T step = 0>
	struct MinMaxSliderBase {

		static constexpr T incrementValue = step ? step : 1;

		T value;

		MinMaxSliderBase() {
		
			if constexpr (min != minValue<T>())
				value = min;

			else if constexpr (max != std::numeric_limits<T>::max())
				value = max;

			else value = 0;
		}

		void setValue(T _value) {

			_value = clamp<T, min, max>(_value);

			if constexpr (step)
				if constexpr (std::is_integral_v<T>)
					_value = _value / step * step;
				else
					_value = oic::Math::floor(_value / step) * step;

			value = _value;
		}

		MinMaxSliderBase(T _value) {
			setValue(_value);
		}

		//Generic operators

		inline MinMaxSliderBase &operator++() {
			value = clamp<T, min, max>(value + incrementValue);
			return *this;
		}

		inline MinMaxSliderBase &operator--() {
			value = clamp<T, min, max>(value - incrementValue);
			return *this;
		}

		inline operator T() const { return value; }

		inline MinMaxSliderBase &operator+=(T t) { setValue(value + t); return *this; }
		inline MinMaxSliderBase &operator-=(T t) { setValue(value - t); return *this; }
		inline MinMaxSliderBase &operator*=(T t) { setValue(value * t); return *this; }
		inline MinMaxSliderBase &operator/=(T t) { setValue(value / t); return *this; }

		inline MinMaxSliderBase operator+(T t) { return MinMaxSliderBase(value + t); }
		inline MinMaxSliderBase operator-(T t) { return MinMaxSliderBase(value - t); }
		inline MinMaxSliderBase operator*(T t) { return MinMaxSliderBase(value * t); }
		inline MinMaxSliderBase operator/(T t) { return MinMaxSliderBase(value / t); }

		//Remainder of division

		inline MinMaxSliderBase operator%(T t) { return MinMaxSliderBase(mod<T>(value, t)); }
		inline MinMaxSliderBase &operator%=(T t) { setValue(mod<T>(value, t)); return *this; }

		//Hat operator is either pow or xor, depending on if it's a float or not

		inline MinMaxSliderBase &operator^=(T t) { setValue(hat<T>(value, t)); return *this; }
		inline MinMaxSliderBase operator^(T t) { return MinMaxSliderBase(hat<T>(value, t)); }

		//Integer only
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase &operator|=(T t) { setValue(value | t); return *this; }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase &operator&=(T t) { setValue(value | t); return *this; }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase operator|(T t) { return MinMaxSliderBase(value | t); }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase operator&(T t) { return MinMaxSliderBase(value & t); }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase &operator>>=(T t) { setValue(value << t); return *this; }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase &operator<<=(T t) { setValue(value >> t); return *this; }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase operator>>(T t) { return MinMaxSliderBase(value >> t); }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase operator<<(T t) { return MinMaxSliderBase(value << t); }
		
		template<typename = std::enable_if_t<std::is_integral_v<T>>>
		inline MinMaxSliderBase operator~() { return MinMaxSliderBase(~value); }

	};

	template<typename T, T min, T max, T step = 0>
	using MinMaxSlider = MinMaxSliderBase<T, min, max, step>;

	template<typename T, T min, T step = 0>
	using MinSlider = MinMaxSlider<T, min, std::numeric_limits<T>::max(), step>;

	template<typename T, T max, T step = 0>
	using MaxSlider = MinMaxSlider<T, minValue<T>(), max, step>;

	template<typename T, T step = 0>
	using Slider = MinMaxSlider<T, minValue<T>(), std::numeric_limits<T>::max(), step>;

	//Progress bar

	template<typename T, T min, T max, T step = 0>
	using MinMaxProgress = MinMaxSliderBase<T, min, max, step>;

	template<typename T, T min, T step = 0>
	using MinProgress = MinMaxProgress<T, min, std::numeric_limits<T>::max(), step>;

	template<typename T, T max, T step = 0>
	using MaxProgress = MinMaxProgress<T, minValue<T>(), max, step>;

	template<typename T, T step = 0>
	using Progress = MinMaxProgress<T, minValue<T>(), std::numeric_limits<T>::max(), step>;

	//Generic value container

	template<typename T>
	struct ValueContainer {

		T value;

		ValueContainer() : value {} {}
		ValueContainer(T _value) : value(_value) {}

		inline operator T() const { return value; }
		inline operator T&() { return value; }

	};

	//Dropdowns and radio buttons (require oicExposedEnum)

	template<typename T, bool isRadioButton>
	struct EnumContainer {

		typename T::BaseType value;

		EnumContainer() : value(T::values[0]) {}
		EnumContainer(T _value) : value(_value) {}

		inline operator T() const { return value; }
		inline operator T&() { return value; }

		EnumContainer(enum T::_E _value) : value(_value) {}

		inline usz id() const {
			return T::idByValue(enum T::_E(value));
		}

		inline void setId(usz id) {
			value = T::values[id];
		}

		static constexpr inline auto values() {
			return T::values;
		}

		static constexpr inline auto names() {
			return T::getCNames();
		}

		static constexpr inline usz valueCount() {
			return T::count;
		}

	};

	template<typename T>
	using RadioButtons = EnumContainer<T, true>;

	template<typename T>
	using Dropdown = EnumContainer<T, false>;

	//Boolean container

	template<typename T, void (T::*x)() const>
	struct Button { 
		static inline void call(const T *v) {
			(v->*x)();
		}
	};

}