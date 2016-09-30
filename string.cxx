#include "string.hxx"
#include <cstring>
#include <new>

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
	const size_t len = vsnprintf(NULL, 0, format, args) + 1;
	auto ret = makeUnique<char []>(len);
	if (!ret)
		return nullptr;
	vsprintf(ret.get(), format, args);
	return std::unique_ptr<const char []>(ret.release());
}

inline bool isMultiValid() noexcept { return true; }
template<typename... values_t> inline bool isMultiValid(const char c, values_t ...values) noexcept
	{ return (c & 0xC0) == 0x80 && isMultiValid(values...); }

template<typename T> inline T safeIndex(const T *const str, const size_t index, const size_t len) noexcept
{
	if (index >= len)
		return -1;
	return str[index];
}

size_t countUnits(const char *const str) noexcept
{
	const size_t len = strlen(str);
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
				// (needs re-commenting)
				else if ((byteA & 0x0F) == 0x0D && (byteB & 0x20))
					return 0;
			}
			else if ((byteA & 0x78) == 0x70)
			{
				// 4 code units.. check that the second, third and fourth unit is valid
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

std::unique_ptr<char16_t []> toUTF16_t::convert(const char *const str) noexcept
{
	const size_t lenUTF8 = strlen(str) + 1;
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
				const char16_t upper = (char16_t(byteA & 0x07) << 8) | (char16_t(byteB & 0x3F) << 2) | (char16_t(byteC & 0x30) >> 8);
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

std::unique_ptr<char []> toUTF16_t::convert(const char16_t *const str) noexcept
{
	//makeUnique<char []>(len);

	return nullptr;
}
