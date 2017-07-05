#include "string.hxx"
#include <cstring>
#include <new>

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
template<typename... values_t> inline bool isMultiValid(const char c, values_t ...values) noexcept
	{ return (c & 0xC0) == 0x80 && isMultiValid(values...); }

/*!
 * @internal
 * @brief Safe array indexing function
 * @returns either the value at the given index, or -1 if outside the bounds of the array
 */
template<typename T> inline T safeIndex(const T *const str, const size_t index, const size_t len) noexcept
{
	if (index >= len)
		return -1;
	return str[index];
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
		const char byteA = str[i];
		// Multiple code unit encoded character?
		if (byteA & 0x80)
		{
			const char byteB = safeIndex(str, ++i, len);
			if ((byteA & 0x60) == 0x40)
			{
				// 2 code units.. check that the second unit is valid and return 0 if not.
				if (!isMultiValid(byteB))
					return 0;
			}
			else if ((byteA & 0x70) == 0x60)
			{
				// 3 code units.. check that the second and third units are valid and return 0 if not
				if (!isMultiValid(byteB, safeIndex(str, ++i, len)))
					return 0;
				// Also check that the code unit is valid (not D800-DF00)
				else if ((byteA & 0x0F) == 0x0D && (byteB & 0x20))
					return 0;
			}
			else if ((byteA & 0x78) == 0x70)
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
		const char16_t uintA = str[i];
		if ((uintA & 0xFE00) == 0xD800)
		{
			const char16_t uintB = safeIndex(str, ++i, len);
			// Ok, should be a surrogate. Check validity..
			if ((uintB & 0xFE00) != 0xDC00)
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
			if (uintA > 0x007F)
				++count;
			// If we're also above 0x07FF, it's 3-byte.
			if (uintA > 0x07FF)
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
		const char byteA = str[i];
		if (byteA & 0x80)
		{
			const char byteB = safeIndex(str, ++i, lenUTF8);
			if ((byteA & 0x60) == 0x40)
				result[j] = (char16_t(byteA & 0x1F) << 6) | char16_t(byteB & 0x3F);
			else if ((byteA & 0x70) == 0x60)
			{
				const char byteC = safeIndex(str, ++i, lenUTF8);
				result[j] = (char16_t(byteA & 0x0F) << 12) | (char16_t(byteB & 0x3F) << 6) | char16_t(byteC & 0x3F);
			}
			else
			{
				const char byteC = safeIndex(str, ++i, lenUTF8);
				const char byteD = safeIndex(str, ++i, lenUTF8);
				// First, collect the upper 11 bits into a value..
				const char16_t upper = (char16_t(byteA & 0x07) << 8) | (char16_t(byteB & 0x3F) << 2) | (char16_t(byteC & 0x30) >> 4);
				// Then take off the (0x010000 >> 10) value, and generate the first part of the surrogate pair
				result[j] = char16_t(0xD800) | (upper - 0x0040);
				// Then collect together the lower 10 bits and generate the second part of the surrogate pair
				result[++j] = char16_t(0xDC00) | (char16_t(byteC & 0x0F) << 6) | char16_t(byteD & 0x3F);
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
		const char16_t uintA = str[i];
		// Surrogate pair?
		if ((uintA & 0xFE00) == 0xD800)
		{
			// Recover the upper 10 (11) bits from the first surrogate of the pair.
			const char16_t upper = (uintA & 0x03FF) + 0x0040;
			// Recover the lower 10 bits from the second surrogate of the pair.
			const char16_t lower = safeIndex(str, ++i, lenUTF16) & 0x03FF;

			result[j] = char(0xF0) | (char(upper >> 8) & 0x07);
			result[++j] = char(0x80) | (char(upper >> 2) & 0x3F);
			result[++j] = char(0x80) | (char(upper << 4) & 0x30) | (char(lower >> 6) & 0x0F);
			result[++j] = char(0x80) | char(lower & 0x3F);
		}
		else
		{
			if (uintA <= 0x007F)
				result[j] = char(uintA);
			else if (uintA <= 0x07FF)
			{
				result[j] = char(0xC0) | (char(uintA >> 6) & 0x1F);
				result[++j] = char(0x80) | char(uintA & 0x3F);
			}
			else
			{
				result[j] = char(0xE0) | (char(uintA >> 12) & 0x0F);
				result[++j] = char(0x80) | (char(uintA >> 6) & 0x3F);
				result[++j] = char(0x80) | char(uintA & 0x3F);
			}
		}
	}
	return result;
}
