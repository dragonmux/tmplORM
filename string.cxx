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
	std::unique_ptr<char []> ret(new (std::nothrow) char[len]);
	if (!ret)
		return nullptr;
	vsprintf(ret.get(), format, args);
	return std::unique_ptr<const char []>(ret.release());
}

inline bool isMultiValid() noexcept { return true; }
template<typename... values_t> inline bool isMultiValid(const char c, values_t ...values) noexcept
	{ return (c & 0xC0) == 0x80 && isMultiValid(values...); }

inline char safeIndex(const char *const str, const size_t index, const size_t len) noexcept
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
				// Also check that the whole only contributes one UTF-16 unit - if it does not, pre-increment count
				if (byteA & 0x0F)
					++count;
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
	const size_t len = countUnits(str);
	auto result = make_unique<char16_t []>(len);

	return nullptr;
}

std::unique_ptr<char []> toUTF16_t::convert(const char16_t *const) noexcept
{
	//make_unique<char []>(len);

	return nullptr;
}
