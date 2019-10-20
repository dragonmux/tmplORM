#ifndef tmplORM_CONVERSIONS__HXX
#define tmplORM_CONVERSIONS__HXX

#include <type_traits>
#include <utility>
#include <string>
#include <string.hxx>

template<typename> struct isBoolean_ : public std::false_type { };
template<> struct isBoolean_<bool> : public std::true_type { };
template<typename T> struct isBoolean : public std::integral_constant<bool,
	isBoolean_<typename std::remove_cv<T>::type>::value> { };

// Type aliases for some type_traits utilities to make them easier to type and read below
template<bool B, typename T = void> using enableIf = typename std::enable_if<B, T>::type;
template<typename A, typename B> using isSame = std::is_same<A, B>;
template<typename A> using isIntegral = std::is_integral<A>;
template<typename A> using isUnsigned = std::is_unsigned<A>;
template<typename A> using isSigned = std::is_signed<A>;
template<typename A> using makeUnsigned = typename std::make_unsigned<A>::type;

template<typename int_t, typename valueType_t> struct fromInt_t
{
private:
	using uint_t = makeUnsigned<int_t>;
	const valueType_t &_value;

	uint8_t calcDigits(const uint_t number) const noexcept
	{
		if (number < 10)
			return 1;
		return 1 + calcDigits(number / 10);
	}

	uint8_t digits(const int_t number) const noexcept
	{
		if (isSigned<int_t>::value && number < 0)
			return 1 + calcDigits(-number);
		return calcDigits(number);
	}

	constexpr size_t power10(const size_t power) const noexcept
		{ return power ? power10(power - 1) * 10 : 1; }

	uint8_t zeros(const int_t number) const noexcept
	{
		const int_t num = number / 10;
		return (num * 10 == number ? 1 + zeros(num) : 0);
	}

	[[gnu::noinline]] uint_t process(const uint_t number, char *const buffer, const uint8_t digits, const size_t index) const noexcept
	{
		if (number < 10)
		{
			buffer[digits - index] = number + '0';
			buffer[digits + 1] = 0;
		}
		else
		{
			const uint_t num = number - (process(number / 10, buffer, digits, index + 1) * 10);
			buffer[digits - index] = num + '0';
		}
		return number;
	}

	void process(const uint_t number, char *const buffer) const noexcept
		{ process(number, buffer, digits(_value) - 1, 0); }
	template<typename T = int_t> enableIf<isSame<T, int_t>::value && isIntegral<T>::value && !isBoolean<T>::value && isUnsigned<T>::value>
		format(char *const buffer) const noexcept { process(_value, buffer); }
	template<typename T = int_t> [[gnu::noinline]] enableIf<isSame<T, int_t>::value && isIntegral<T>::value && !isBoolean<T>::value && isSigned<T>::value>
		format(char *const buffer) const noexcept
	{
		int_t number = _value;
		if (number < 0)
		{
			buffer[0] = '-';
			process(uint_t(-number), buffer);
		}
		else
			process(uint_t(number), buffer);
	}

	void formatFraction(const uint8_t maxDigits, char *const buffer) const noexcept
	{
		const uint8_t digits_ = digits();
		if (digits_ > maxDigits)
			process(_value - ((_value / power10(maxDigits)) * power10(maxDigits)), buffer, maxDigits - 1, 0);
		else
		{
			const uint8_t trailingZeros_ = trailingZeros();
			const uint8_t leadingZeros = maxDigits - digits_;
			for (uint8_t i{0}; i < leadingZeros; ++i)
				buffer[i] = '0';
			process(_value / power10(trailingZeros_), buffer + leadingZeros, digits_ - trailingZeros_ - 1, 0);
		}
	}

public:
	constexpr fromInt_t(const valueType_t &value) noexcept : _value(value) { }

	operator std::unique_ptr<char []>() const noexcept
	{
		auto number = makeUnique<char []>(digits(_value) + 1);
		if (!number)
			return nullptr;
		format(number.get());
		return number;
	}

	operator const char *() const noexcept
	{
		std::unique_ptr<char []> number = *this;
		return number.release();
	}

	uint8_t digits() const noexcept { return digits(_value); }
	uint8_t length() const noexcept { return digits() + 1; }
	void formatTo(char *const buffer) const noexcept { format(buffer); }

	std::unique_ptr<char []> formatFraction(const uint8_t maxDigits) const noexcept
	{
		auto number = makeUnique<char []>(fractionLength(maxDigits));
		if (!number)
			return nullptr;
		formatFraction(maxDigits, number.get());
		return number;
	}

	uint8_t fractionDigits(const uint8_t maxDigits) const noexcept
	{
		const uint8_t digits_ = digits();
		if (digits_ > maxDigits)
			return maxDigits;
		return (maxDigits - digits_) + (digits_ - trailingZeros());
	}

	uint8_t trailingZeros() const noexcept { return _value ? zeros(_value) : 0; }
	uint8_t fractionLength(const uint8_t maxDigits) const noexcept { return fractionDigits(maxDigits) + 1; }
	void formatFractionTo(const uint8_t maxDigits, char *const buffer) const noexcept
		{ formatFraction(maxDigits, buffer); }
};

template<typename int_t> struct toInt_t
{
private:
	using uint_t = makeUnsigned<int_t>;
	const char *const _value;
	const size_t _length;
	constexpr static const bool _isSigned = isSigned<int_t>::value;

	static bool isNumber(const char x) noexcept { return x >= '0' && x <= '9'; }
	static bool isHex(const char x) noexcept { return isNumber(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'); }
	static bool isOct(const char x) noexcept { return x >= '0' && x <= '7'; }

	template<bool isFunc(const char)> bool checkValue() const noexcept
	{
		for (size_t i = 0; i < _length; ++i)
		{
			if (!isFunc(_value[i]))
				return false;
		}
		return true;
	}

public:
	toInt_t(const char *const value) noexcept : _value(value), _length(std::char_traits<char>::length(value)) { }
	constexpr toInt_t(const char *const value, const size_t subLength) noexcept : _value(value), _length(subLength) { }
	size_t length() const noexcept { return _length; }

	bool isInt() const noexcept
	{
		if (_isSigned && _value[0] == '-' && length() == 1)
			return false;
		for (size_t i = 0; i < _length; ++i)
		{
			if (_isSigned && i == 0 && _value[i] == '-')
				continue;
			else if (!isNumber(_value[i]))
				return false;
		}
		return true;
	}

	bool isHex() const noexcept { return checkValue<isHex>(); }
	bool isOct() const noexcept { return checkValue<isOct>(); }
	int_t fromInt() const noexcept { return *this; }

	operator int_t() const noexcept
	{
		uint_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			if (_isSigned && i == 0 && _value[i] == '-')
				continue;
			value *= 10;
			value += _value[i] - '0';
		}
		if (_isSigned && _value[0] == '-')
			return -int_t(value);
		return int_t(value);
	}

	int_t fromHex() const noexcept
	{
		int_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			uint8_t hex(_value[i]);
			if (!isHex(hex))
				return {};
			else if (hex >= 'a' && hex <= 'f')
				hex -= 0x20;
			value <<= 4;
			hex -= 0x30;
			if (hex > 9)
				hex -= 0x07;
			value += hex;
		}
		return value;
	}

	int_t fromOct() const noexcept
	{
		int_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			if (!isOct(_value[i]))
				return {};
			value <<= 3;
			value += _value[i] - 0x30;
		}
		return value;
	}
};

inline void swapBytes(uint16_t &val) noexcept
{
	val = (uint16_t(val >> 8U) & 0xFFU) | uint16_t((val & 0xFFU) << 8U);
}

inline void swapBytes(uint32_t &val) noexcept
{
	val = (uint32_t(val >> 24U) & 0xFFU) | (uint32_t(val >> 8U) & 0xFF00U) |
		uint32_t((val & 0xFF00U) << 8U) | uint32_t((val & 0xFFU) << 24U);
}

inline void swapBytes(uint64_t &val) noexcept
{
	val = (uint64_t(val >> 56U) & 0xFFU) | (uint64_t(val >> 40U) & 0xFF00U) | (uint64_t(val >> 24U) & 0xFF0000U) | (uint64_t(val >> 8U) & 0xFF000000U) |
		uint64_t((val & 0xFF000000U) << 8U) | uint64_t((val & 0xFF0000U) << 24U) | uint64_t((val & 0xFF00U) << 40U) | uint64_t((val & 0xFFU) << 56U);
}

template<typename T> inline T swapBytes(const T &a) noexcept
{
	typename std::make_unsigned<T>::type result(a);
	swapBytes(result);
	return result;
}

inline char asHex(const uint8_t value) noexcept
	{ return value < 10 ? value + '0' : value + 'A' - 10; }

#endif /*tmplORM_CONVERSIONS__HXX*/
