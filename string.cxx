#include "string.hxx"
#include <cstring>
#include <new>
#include <limits>

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2017
 * @brief Implementation of various string helpers which ideally would be in the STL
 */

std::unique_ptr<const char []> formatString(const char *format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	auto ret = vaFormatString(format, args);
	va_end(args);
	return ret;
}

std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept
{
	va_list lenArgs;
	va_copy(lenArgs, args);
	const size_t len = vsnprintf(nullptr, 0, format, lenArgs) + 1;
	va_end(lenArgs);
	auto ret = makeUnique<char []>(len);
	if (!ret)
		return nullptr;
	vsprintf(ret.get(), format, args);
	return std::unique_ptr<const char []>(ret.release());
}

std::unique_ptr<char []> stringDup(const char *const str) noexcept
{
	auto ret = makeUnique<char []>(strlen(str) + 1);
	if (!ret)
		return nullptr;
	strcpy(ret.get(), str);
	return ret;
}

std::unique_ptr<const char []> strNewDup(const char *const str) noexcept
	{ return std::unique_ptr<const char []>(stringDup(str).release()); }

inline bool isMultiValid() noexcept { return true; }
template<typename... values_t> inline bool isMultiValid(const uint8_t c, values_t ...values) noexcept
	{ return (c & 0xC0U) == 0x80U && isMultiValid(values...); }

/*!
 * @internal
 * @brief Safe array indexing function
 * @returns either the value at the given index, or -1 if outside the bounds of the array
 */
template<typename T, typename U = typename std::make_unsigned<T>::type>
	inline U safeIndex(const T *const str, const size_t index, const size_t len) noexcept
{
	if (index >= len)
		return std::numeric_limits<U>::max();
	U result{};
	memcpy(&result, &str[index], sizeof(T));
	return result;
}

/*!
 * @internal
 * @brief Counts the number of code units needed to represent the input UTF-8 string as a UTF-16 string
 * @param str The input UTF-8 encoded unicode string
 * @returns the number of code units required, or 0 if there was an error in the input string
 * @sa http://en.wikipedia.org/wiki/UTF-8
 */
size_t countUnits(const char *const str) noexcept
{
	const size_t len = utf16::length(str);
	size_t count = 0;
	for (size_t i = 0; i < len; ++i)
	{
		const auto byteA = safeIndex(str, i, len);
		// Multiple code unit encoded character?
		if (byteA & 0x80U)
		{
			const auto byteB = safeIndex(str, ++i, len);
			if ((byteA & 0x60U) == 0x40U)
			{
				// 2 code units.. check that the second unit is valid and return 0 if not.
				if (!isMultiValid(byteB))
					return 0;
			}
			else if ((byteA & 0x70U) == 0x60U)
			{
				// 3 code units.. check that the second and third units are valid and return 0 if not
				if (!isMultiValid(byteB, safeIndex(str, ++i, len)))
					return 0;
				// Also check that the code unit is valid (not D800-DF00)
				else if ((byteA & 0x0FU) == 0x0DU && (byteB & 0x20U))
					return 0;
			}
			else if ((byteA & 0x78U) == 0x70U)
			{
				// 4 code units.. check that the second, third and fourth units are valid
				if (!isMultiValid(byteB, safeIndex(str, i + 1, len), safeIndex(str, i + 2, len)))
					return 0;
				i += 2;
				++count;
			}
			else
				return 0;
		}
		++count;
	}
	return count;
}

/*!
 * @internal
 * @brief Counts the number of code units needed to represent the input UTF-16 string as a UTF-8 string
 * @param str The input UTF-16 encoded unicode string
 * @returns the number of code units required, or 0 if there was an error in the input string
 * @sa http://en.wikipedia.org/wiki/UTF-16
 */
size_t countUnits(const char16_t *const str) noexcept
{
	const size_t len = utf16::length(str);
	size_t count = 0;
	for (size_t i = 0; i < len; ++i)
	{
		const auto uintA = safeIndex(str, i, len);
		if ((uintA & 0xFE00U) == 0xD800U)
		{
			const auto uintB = safeIndex(str, ++i, len);
			// Ok, should be a surrogate. Check validity..
			if ((uintB & 0xFE00U) != 0xDC00U)
				return 0;
			// Guaranteed this is 4-byte.
			count += 3;
		}
		// It is invalid to encounter a floating secondary surrogate
		else if ((uintA & 0xFE00) == 0xDC00)
			return 0;
		else
		{
			// Nope.. well.. let's do the checks then.
			// If we have a value more than 0x007F, we're guaranteed multi-byte.
			if (uintA > 0x007FU)
				++count;
			// If we're also above 0x07FF, it's 3-byte.
			if (uintA > 0x07FFU)
				++count;
		}
		++count;
	}
	return count;
}

utf16_t utf16::convert(const char *const str) noexcept
{
	const size_t lenUTF8 = utf16::length(str);
	const size_t lenUTF16 = countUnits(str);
	auto result = makeUnique<char16_t []>(lenUTF16);
	if (!result || !lenUTF16)
		return nullptr;
	for (size_t i = 0, j = 0; i < lenUTF8; ++i, ++j)
	{
		const auto byteA = safeIndex(str, i, lenUTF8);
		if (byteA & 0x80U)
		{
			const auto byteB = safeIndex(str, ++i, lenUTF8);
			if ((byteA & 0x60U) == 0x40U)
				result[j] = char16_t((byteA & 0x1FU) << 6U) | char16_t(byteB & 0x3FU);
			else if ((byteA & 0x70U) == 0x60U)
			{
				const auto byteC = safeIndex(str, ++i, lenUTF8);
				result[j] = char16_t(uint16_t((byteA & 0x0FU) << 12U) | uint16_t((byteB & 0x3FU) << 6U) | (byteC & 0x3FU));
			}
			else
			{
				const auto byteC = safeIndex(str, ++i, lenUTF8);
				const auto byteD = safeIndex(str, ++i, lenUTF8);
				// First, collect the upper 11 bits into a value..
				const auto upper = uint16_t(uint8_t(byteA & 0x07U) << 8U) | uint16_t(uint8_t(byteB & 0x3FU) << 2U) |
					uint16_t(uint8_t(byteC & 0x30U) >> 4U);
				// Then take off the (0x010000 >> 10) value, and generate the first part of the surrogate pair
				result[j] = char16_t(0xD800U | (upper - 0x0040U));
				// Then collect together the lower 10 bits and generate the second part of the surrogate pair
				result[++j] = char16_t(0xDC00U | uint16_t((byteC & 0x0FU) << 6U) | (byteD & 0x3FU));
			}
		}
		else
			result[j] = char16_t(byteA);
	}
	return result;
}

utf8_t utf16::convert(const char16_t *const str) noexcept
{
	const size_t lenUTF16 = utf16::length(str);
	const size_t lenUTF8 = countUnits(str);
	auto result = makeUnique<char []>(lenUTF8);
	if (!result || !lenUTF8)
		return nullptr;
	for (size_t i = 0, j = 0; i < lenUTF16; ++i, ++j)
	{
		const auto uintA = safeIndex(str, i, lenUTF16);
		// Surrogate pair?
		if ((uintA & 0xFE00U) == 0xD800U)
		{
			// Recover the upper 10 (11) bits from the first surrogate of the pair.
			const auto upper = (uintA & 0x03FFU) + 0x0040U;
			// Recover the lower 10 bits from the second surrogate of the pair.
			const auto lower = safeIndex(str, ++i, lenUTF16) & 0x03FFU;

			result[j] = char(0xF0U | (uint8_t(upper >> 8U) & 0x07U));
			result[++j] = char(0x80U | (uint8_t(upper >> 2U) & 0x3FU));
			result[++j] = char(0x80U | (uint8_t(upper << 4U) & 0x30U) | (uint8_t(lower >> 6U) & 0x0FU));
			result[++j] = char(0x80U | (lower & 0x3FU));
		}
		else
		{
			if (uintA <= 0x007FU)
				result[j] = char(uintA);
			else if (uintA <= 0x07FFU)
			{
				result[j] = char(0xC0U | (uint8_t(uintA >> 6U) & 0x1FU));
				result[++j] = char(0x80U | uint8_t(uintA & 0x3FU));
			}
			else
			{
				result[j] = char(0xE0U | (uint8_t(uintA >> 12U) & 0x0FU));
				result[++j] = char(0x80U | (uint8_t(uintA >> 6U) & 0x3FU));
				result[++j] = char(0x80U | (uintA & 0x3FU));
			}
		}
	}
	return result;
}
